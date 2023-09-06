//
// Created by Yoshiki on 2023/8/7.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
#define LINUX_SERVER_LIB_KV_STORE_EPOLL_H_

#include <functional>
#include <sys/epoll.h>
#include "printf-color.h"
#include <thread>
#include <fcntl.h>
#include <list>
#include <algorithm>
#include <mutex>
#include <atomic>
#include "kv-command.h"

#define MAX_EPOLL_ONCE_NUM 1024
#define MESSAGE_SIZE_MAX 65535

#define SET_COMMON_EPOLL_FLAG(event) ((event) | EPOLLHUP | EPOLLRDHUP)

template <int>
class Tcp;
struct SockEpollPtr
{
    enum QUEUE_STATUS
    {
        STATUS_LISTEN,
        STATUE_SLEEP,
        STATUS_RECV,
        STATUS_RECV_DOWN,
        STATUS_RECV_BAD,
        STATUS_SEND,
        STATUS_SEND_DOWN,
        STATUS_SEND_BAD
    };
    explicit SockEpollPtr (int fd)
        : fd(fd), sockaddr(sockaddr_in()) {}
    SockEpollPtr (int fd, sockaddr_in &sockaddrIn)
        : fd(fd), sockaddr(sockaddrIn) {}
    int fd;
    struct sockaddr_in sockaddr;
    ResValueType resValue {};
    CommandParams commandParams {};
    QUEUE_STATUS status = STATUS_LISTEN;
    std::string msg {};
};
struct SockEvents
{
    int sockfdLen;
    struct epoll_event epollEvents[MAX_EPOLL_ONCE_NUM];
};
struct ReactorParams
{
    // int epfd;
    std::chrono::milliseconds expireTime;
    std::chrono::milliseconds runtime { 0 };
    void *arg = nullptr;
    SockEvents sockEvents {};

    explicit ReactorParams (std::chrono::milliseconds expireTime)
        : expireTime(expireTime) {}

    void updateTimeNow ()
    {
        runtime = std::chrono::duration_cast <std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }
};
class Reactor
{
protected:
    using ParamsType = SockEpollPtr;
    using Callback = std::function <void (ParamsType &)>;

public:
    template <class T>
    Reactor (T &&acceptCb, T &&recvCb, T &&sendCb)
        : acceptCb_(std::forward <T>(acceptCb)),
          recvCb_(std::forward <T>(recvCb)),
          sendCb_(std::forward <T>(sendCb)) {}

    virtual ~Reactor () noexcept = default;

    inline void accept (ParamsType &params) const
    {
        acceptCb_(params);
    }
    inline void recv (ParamsType &params) const
    {
        recvCb_(params);
    }
    inline void send (ParamsType &params) const
    {
        sendCb_(params);
    }
private:
    Callback acceptCb_;
    Callback recvCb_;
    Callback sendCb_;
};

class EventPoll
{
public:
    explicit EventPoll (int size = 1024)
        : epfd(epoll_create(size))
    {
        // 用于退出epoll_wait 阻塞
        unixfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        event.data.fd = unixfd;
        event.events = EPOLLIN;

        epoll_ctl(epfd, EPOLL_CTL_ADD, unixfd, &event);
    }

    virtual ~EventPoll () noexcept
    {
        close();
    }

    void epollAddEvent (SockEpollPtr &params)
    {
        event.data.ptr = &params;
        event.events = SET_COMMON_EPOLL_FLAG(EPOLLIN);

        epoll_ctl(epfd, EPOLL_CTL_ADD, params.fd, &event);

        allEpollPtr.emplace_back(&params);
    }
    void epollModEvent (SockEpollPtr &params, int events)
    {
        event.data.ptr = &params;
        event.events = SET_COMMON_EPOLL_FLAG(events);

        epoll_ctl(epfd, EPOLL_CTL_MOD, params.fd, &event);
    }
    void epollDelEvent (SockEpollPtr &params)
    {
        event.data.ptr = &params;
        event.events = 0;

        epoll_ctl(epfd, EPOLL_CTL_DEL, params.fd, &event);
        allEpollPtr.erase(
            std::remove(allEpollPtr.begin(), allEpollPtr.end(), &params),
            allEpollPtr.end());
    }
    int epollWait (ReactorParams &params) // NOLINT
    {
        long expireTime = params.expireTime.count() == 0 ? -1 : params.expireTime.count();
        params.updateTimeNow();

        int ret = epoll_wait(
            epfd,
            params.sockEvents.epollEvents,
            MAX_EPOLL_ONCE_NUM,
            static_cast<int>(expireTime));

        if (ret == 0)
        {
            // 超时
        }
        else if (ret == -1)
        {
            if (errno == EINTR)
                return epollWait(params);

            PRINT_ERROR("epoll_wait error : %s", std::strerror(errno));
            return -errno;
        }
        else
            params.sockEvents.sockfdLen = ret;

        return ret;
    }

    void close ()
    {
        event.data.fd = unixfd;
        event.events = EPOLLOUT;
        epoll_ctl(epfd, EPOLL_CTL_MOD, unixfd, &event);

        for (const auto &item : allEpollPtr)
        {
            ::close(item->fd);
            delete item;
        }

        ::close(unixfd);
        ::close(epfd);
    }

private:
    int epfd;
    int unixfd;
    struct epoll_event event {};
    std::vector <SockEpollPtr *> allEpollPtr {};
};

class IoReactor : protected Reactor
{
protected:
    using SockfdQueueType = std::list <SockEpollPtr *>;
public:
    IoReactor ()
        : Reactor(
        std::bind(&IoReactor::acceptCallback, this, std::placeholders::_1),
        std::bind(&IoReactor::recvCallback, this, std::placeholders::_1),
        std::bind(&IoReactor::sendCallback, this, std::placeholders::_1)) {}

    ~IoReactor () noexcept override = default;

    inline SockfdQueueType &sockfdQueue () noexcept
    {
        return sockfdQueue_;
    }
    inline void setStatus (SockEpollPtr::QUEUE_STATUS status) noexcept
    {
        status_ = status;
    }

    void ioHandler ()
    {
        if (!sockfdQueue_.empty())
            switch (status_)
            {
                case SockEpollPtr::STATUS_RECV:
                    recvSocketHandler();
                    setStatus(SockEpollPtr::STATUS_RECV_DOWN);
                    break;
                case SockEpollPtr::STATUS_SEND:
                    sendSocketHandler();
                    setStatus(SockEpollPtr::STATUS_SEND_DOWN);
                    break;
                case SockEpollPtr::STATUS_RECV_DOWN:
                case SockEpollPtr::STATUS_SEND_DOWN:
                case SockEpollPtr::STATUE_SLEEP:
                case SockEpollPtr::STATUS_RECV_BAD:
                case SockEpollPtr::STATUS_LISTEN:
                case SockEpollPtr::STATUS_SEND_BAD:
                    break;
            }
    }

protected:
    template <class T>
    IoReactor (T &&acceptCb, T &&recvCb, T &&sendCb)
        : Reactor(
        std::forward <T>(acceptCb),
        std::forward <T>(recvCb),
        std::forward <T>(sendCb)) {}

public:
    void mainLoop (
        SockfdQueueType &queue,
        std::mutex &mutex,
        bool &terminate
    )
    {
        PRINT_INFO("thread %lu is running...", pthread_self());
        while (!terminate)
        {
            for (int i = 0; i < defaultLoopNum; ++i)
            {
                if (!sockfdQueue().empty())
                    break;
            }

            if (sockfdQueue().empty())
            {
                // 休眠
                mutex.lock();
                mutex.unlock();
                continue;
            }

            ioHandler();
            mutex.lock();
            // 合并任务
            queue.merge(std::move(sockfdQueue_));
            sockfdQueue_.clear();
            mutex.unlock();
        }
    }
private:
    inline void recvSocketHandler ()
    {
        if (!sockfdQueue().empty())
            for (SockEpollPtr *sockParams : sockfdQueue())
                recv(*sockParams);
    }

    inline void sendSocketHandler ()
    {
        if (!sockfdQueue().empty())
            for (SockEpollPtr *sockParams : sockfdQueue())
                send(*sockParams);
    }

    void acceptCallback (ParamsType &fnParams)
    {

    }
    void recvCallback (ParamsType &fnParams) // NOLINT
    {
        int ret;
        char message[MESSAGE_SIZE_MAX];
        do
        {
            ret = ::recv(fnParams.fd, message, MESSAGE_SIZE_MAX, 0);
            // 不做处理  留给mainReactor去处理
            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                PRINT_ERROR("recv error : %s", std::strerror(errno));
                fnParams.status = SockEpollPtr::STATUS_RECV_BAD;
                break;
            }
            else if (ret == 0)
            {
                PRINT_INFO("pipe close : %s, sockfd : %d",
                    Utils::getIpAndHost(fnParams.sockaddr).c_str(),
                    fnParams.fd);
                fnParams.status = SockEpollPtr::STATUS_RECV_BAD;
                break;
            }
            else
            {
                message[ret] = '\0';
                PRINT_INFO("get client command : %s,  addr : %s, sockfd : %d, thread : %lu",
                    message,
                    Utils::getIpAndHost(fnParams.sockaddr).c_str(),
                    fnParams.fd,
                    pthread_self());

                fnParams.msg = message;
                // 命令解析
                fnParams.commandParams = CommandHandler::splitCommandParams(fnParams.msg);
                fnParams.status = SockEpollPtr::STATUS_RECV_DOWN;
            }
        } while (ret <= 0);
    }

    void sendCallback (ParamsType &fnParams) // NOLINT
    {
        int ret;
        std::string resMessage = formatResValue(fnParams.resValue);

        do
        {
            ret = ::send(fnParams.fd, resMessage.c_str(), resMessage.size(), 0);
            // 不做处理  留给mainReactor去处理
            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                PRINT_ERROR("send error : %s", std::strerror(errno));
                fnParams.status = SockEpollPtr::STATUS_SEND_BAD;
                break;
            }
            else if (ret == 0)
            {
                PRINT_INFO("pipe close : %s, sockfd : %d",
                    Utils::getIpAndHost(fnParams.sockaddr).c_str(),
                    fnParams.fd);
                fnParams.status = SockEpollPtr::STATUS_SEND_BAD;
                break;
            }
            else
            {
                PRINT_INFO(
                    "reply for command \"%s\", message : %s, addr : %s, sockfd : %d, thread : %lu",
                    fnParams.msg.c_str(),
                    resMessage.c_str(),
                    Utils::getIpAndHost(fnParams.sockaddr).c_str(),
                    fnParams.fd,
                    pthread_self());

                fnParams.status = SockEpollPtr::STATUS_SEND_DOWN;
            }
        } while (ret <= 0);
    }

    static std::string formatResValue (const ResValueType &resValue)
    {
        std::string resMessage;
        int idx = 0;
        static std::stringstream stringFormatter;
        switch (resValue.model)
        {
            case ResValueType::ReplyModel::REPLY_UNKNOWN:
                break;
            case ResValueType::ReplyModel::REPLY_INTEGER:
                resMessage = "(integer) ";
                resMessage += resValue.value;
                break;
            case ResValueType::ReplyModel::REPLY_STATUS:
            case ResValueType::ReplyModel::REPLY_NIL:
            case ResValueType::ReplyModel::REPLY_ERROR:
            case ResValueType::ReplyModel::REPLY_STRING:
                resMessage = resValue.value;
                break;
            case ResValueType::ReplyModel::REPLY_ARRAY:
                if (resValue.elements.empty())
                {
                    resMessage = "(empty array)";
                    break;
                }
                stringFormatter.str("");
                for (auto &v : resValue.elements)
                    stringFormatter << ++idx << ") \"" << formatResValue(v) << std::endl;
                resMessage = stringFormatter.str();
                break;
        }

        return resMessage;
    }

    static constexpr int defaultLoopNum = 1000000;

    SockfdQueueType sockfdQueue_;
    SockEpollPtr::QUEUE_STATUS status_ = SockEpollPtr::STATUS_RECV;
};

template <int IoThreadNum>
class MainReactor : IoReactor, EventPoll
{
    friend class Tcp <IoThreadNum>;
public:
    explicit MainReactor (
        int listenfd,
        std::chrono::milliseconds expireTime = std::chrono::milliseconds { 0 }
    )
        : IoReactor(),
          EventPoll(), params(expireTime), listenfd(listenfd) {}

    ~MainReactor () noexcept override
    {
        terminate = true;
        mutex.unlock();
        ::close(listenfd);

        for (int i = 0; i < IoThreadNum; ++i)
            ioThread[i].join();
    }

    void mainLoop ()
    {
        auto *epollPtr = new SockEpollPtr(listenfd);
        epollAddEvent(*epollPtr);
        int ret;
        struct epoll_event *event;

        // 上锁让线程休眠
        // distributeSendSocket distributeRecvSocket 解锁让线程处理send recv 后，收集处理好的fd，继续上锁
        mutex.lock();
        setIoThread();
        while (!terminate)
        {
            ret = epollWait(params);
            if (terminate)
                return;

            // 每次epoll_wait返回后检查一次过期的key
            commandHandler.checkExpireKeys();

            onceLoopRecvSum = 0;
            onceLoopSendSum = 0;
            for (int i = 0; i < ret; ++i)
            {
                event = &params.sockEvents.epollEvents[i];
                epollPtr = static_cast<SockEpollPtr *>(event->data.ptr);
                if (event->events & EPOLLRDHUP)
                {
                    PRINT_INFO("对端关闭， addr : %s , fd : %d",
                        Utils::getIpAndHost(epollPtr->sockaddr).c_str(),
                        epollPtr->fd);
                    closeSock(*epollPtr);
                }
                else if (event->events & EPOLLHUP)
                {
                    PRINT_WARNING("连接发生挂起 : %d", epollPtr->fd);
                    closeSock(*epollPtr);
                }
                else if (epollPtr->fd == listenfd)
                    acceptCallback(*epollPtr);
                else if (event->events & EPOLLIN)
                    recvCallback(*epollPtr);
                else if (event->events & EPOLLOUT)
                    sendCallback(*epollPtr);
            }

            if (onceLoopSendSum)
                distributeSendSocket();
            if (onceLoopRecvSum)
                distributeRecvSocket();
        }
    }
private:
    void setIoThread ()
    {
        for (int i = 0; i < IoThreadNum; ++i)
        {
            ioThread[i] = std::move(
                std::thread(
                    &IoReactor::mainLoop,
                    &ioReactor[i],
                    std::ref(sockfdQueue()),
                    std::ref(mutex),
                    std::ref(terminate)
                ));
        }
    }
    void acceptCallback (ParamsType &fnParams)
    {
        static socklen_t len = sizeof(struct sockaddr);
        int fd = ::accept(listenfd, (struct sockaddr *)&clientAddr, &len);
        if (fd == -1)
        {
            PRINT_ERROR("accept error : %s\n", std::strerror(errno));
            return;
        }

        setSockReuseAddr(fd);
        auto *sockParams = new SockEpollPtr(fd, clientAddr);
        epollAddEvent(*sockParams);

        PRINT_INFO("accept by %s:%d , fd : %d",
            inet_ntoa(clientAddr.sin_addr),
            ntohs(clientAddr.sin_port),
            fd);
    }
    void recvCallback (ParamsType &fnParams)
    {
        fnParams.status = SockEpollPtr::STATUS_RECV;
        sockfdQueue().emplace_back(&fnParams);
        onceLoopRecvSum++;
    }
    void sendCallback (ParamsType &fnParams)
    {
        fnParams.status = SockEpollPtr::STATUS_SEND;
        sockfdQueue().emplace_back(&fnParams);
        onceLoopSendSum++;
    }

    void distributeRecvSocket ()
    {
        setStatus(SockEpollPtr::STATUS_RECV);
        for (int i = 0; i < IoThreadNum; ++i)
            ioReactor[i].setStatus(SockEpollPtr::STATUS_RECV);

        distributeTask(onceLoopRecvSum);

        recvAfterHandler();
        // 123
    }

    void distributeSendSocket ()
    {
        setStatus(SockEpollPtr::STATUS_SEND);
        for (int i = 0; i < IoThreadNum; ++i)
            ioReactor[i].setStatus(SockEpollPtr::STATUS_SEND);

        distributeTask(onceLoopSendSum);

        sendAfterHandler();
    }

    inline void distributeTask (size_t num)
    {
        static SockfdQueueType::iterator nextIt;
        // 分发任务
        for (auto it = sockfdQueue().begin(); it != sockfdQueue().end(); ++it)
        {
            IoReactor &reactor = getRandomReactor();
            if (&reactor != this)
            {
                nextIt = std::next(it);
                reactor.sockfdQueue().splice(reactor.sockfdQueue().end(), sockfdQueue(), it);
                it = nextIt;
            }
        }
        // 解锁让线程去处理fd
        mutex.unlock();
        ioHandler();

        while (sockfdQueue().size() != num)
            std::this_thread::sleep_for(std::chrono::microseconds { 10 });

        // 让io线程休眠
        mutex.lock();
    }

    inline void recvAfterHandler ()
    {
        for (SockEpollPtr *sockEpollPtr : sockfdQueue())
        {
            // 处理命令
            PRINT_INFO("get message : %s", sockEpollPtr->msg.c_str());

            sockEpollPtr->resValue = commandHandler.handlerCommand(sockEpollPtr->commandParams);
            epollModEvent(*sockEpollPtr, SET_COMMON_EPOLL_FLAG(EPOLLOUT));
        }

        sockfdQueue().clear();
    };

    inline void sendAfterHandler ()
    {
        for (SockEpollPtr *sockEpollPtr : sockfdQueue())
            epollModEvent(*sockEpollPtr, SET_COMMON_EPOLL_FLAG(EPOLLIN));

        sockfdQueue().clear();
    }

    inline void closeSock (ParamsType &fnParams) noexcept
    {
        epollDelEvent(fnParams);
        ::close(fnParams.fd);
        delete &fnParams;
    }

    inline IoReactor &getRandomReactor ()
    {
        static size_t index = 0;
        size_t randomReactor = index++ % (IoThreadNum + 1);
        if (randomReactor == 4)
        {
            return *this;
        }
        return ioReactor[randomReactor];
    }

    static inline void setSockReuseAddr (int sockfd)
    {
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int));
    }

private:
    bool terminate = false;
    ReactorParams params;
    int listenfd;
    struct sockaddr_in clientAddr {};
    std::mutex mutex;
    size_t onceLoopRecvSum = 0;
    size_t onceLoopSendSum = 0;
    CommandHandler commandHandler;

    IoReactor ioReactor[IoThreadNum];
    std::thread ioThread[IoThreadNum];
};
#endif //LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
