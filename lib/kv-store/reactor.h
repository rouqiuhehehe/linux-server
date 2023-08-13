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

template <int>
class Tcp;
struct SockEpollPtr
{
    enum QUEUE_STATUS
    {
        STATUE_SLEEP,
        STATUS_RECV,
        STATUS_RECV_DOWN,
        STATUS_RECV_BAD,
        STATUS_SEND,
        STATUS_SEND_DOWN
    };
    explicit SockEpollPtr (int fd)
        : fd(fd), sockaddr(sockaddr_in()) {}
    SockEpollPtr (int fd, sockaddr_in &sockaddrIn)
        : fd(fd), sockaddr(sockaddrIn) {}
    int fd;
    struct sockaddr_in sockaddr;

    QUEUE_STATUS status = STATUE_SLEEP;
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
        event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

        epoll_ctl(epfd, EPOLL_CTL_ADD, params.fd, &event);

        allEpollPtr.emplace_back(&params);
    }
    void epollModEvent (SockEpollPtr &params, int events)
    {
        event.data.ptr = &params;
        event.events = events | EPOLLHUP | EPOLLRDHUP;

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
                    break;
                case SockEpollPtr::STATUS_RECV_DOWN:
                    break;
                case SockEpollPtr::STATUS_SEND_DOWN:
                    break;
                case SockEpollPtr::STATUE_SLEEP:
                    break;
                case SockEpollPtr::STATUS_RECV_BAD:
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
            queue.merge(std::move(sockfdQueue_));
            sockfdQueue_.clear();
            mutex.unlock();
        }
    }
private:
    void recvSocketHandler ()
    {
        if (!sockfdQueue_.empty())
            for (SockEpollPtr *sockParams : sockfdQueue_)
                recv(*sockParams);
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
                fnParams.status = SockEpollPtr::STATUS_RECV_DOWN;
            }
        } while (ret <= 0);
    }
    void sendCallback (ParamsType &fnParams)
    {

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

        setIoThread();
        terminate = true;
        while (!terminate)
        {
            // 上一次循环只有accept, 不需要重新锁
            if (onceLoopRecvSum != 0 && onceLoopSendSum != 0)
                mutex.lock();
            ret = epollWait(params);
            if (terminate)
                return;

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
        IoReactor &reactor = getRandomReactor();
        fnParams.status = SockEpollPtr::STATUS_RECV;
        reactor.sockfdQueue().emplace_back(&fnParams);
        onceLoopRecvSum++;
    }
    void sendCallback (ParamsType &fnParams)
    {

    }

    void distributeRecvSocket ()
    {
        setStatus(SockEpollPtr::STATUS_RECV);
        for (int i = 0; i < IoThreadNum; ++i)
            ioReactor[i].setStatus(SockEpollPtr::STATUS_RECV);

        // 解锁让线程去处理recv
        mutex.unlock();

        ioHandler();

        while (sockfdQueue().size() != onceLoopRecvSum)
            std::this_thread::sleep_for(std::chrono::microseconds { 100 });

        recvCommandHandler(*this);
        // 123
    }

    inline void recvCommandHandler (IoReactor &ioQueue)
    {
        ioQueue.setStatus(SockEpollPtr::STATUS_RECV_DOWN);
        for (SockEpollPtr *sockEpollPtr : ioQueue.sockfdQueue())
        {
            // 处理命令
            PRINT_INFO("get message : %s", sockEpollPtr->msg.c_str());
        }

        ioQueue.sockfdQueue().clear();
    };

    inline void closeSock (ParamsType &fnParams) noexcept
    {
        epollDelEvent(fnParams);
        ::close(fnParams.fd);
        delete &fnParams;
    }

    inline IoReactor &getRandomReactor ()
    {
        static size_t index = 0;
        size_t randomReactor = ++index % (IoThreadNum + 1);
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
    int onceLoopRecvSum = 0;
    int onceLoopSendSum = 0;

    IoReactor ioReactor[IoThreadNum];
    std::thread ioThread[IoThreadNum];
};
#endif //LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
