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
#include <queue>
#include <algorithm>
#include <mutex>

#define MAX_EPOLL_ONCE_NUM 1024
#define MESSAGE_SIZE_MAX 65535
struct SockEpollPtr
{
    explicit SockEpollPtr (int fd)
        : fd(fd) {}
    int fd;
    struct sockaddr_in sockaddr {};
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
        : epfd(epoll_create(size)) {}

    void epollAddEvent (SockEpollPtr &params)
    {
        event.data.ptr = &params;
        event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

        epoll_ctl(epfd, EPOLL_CTL_ADD, params.fd, &event);
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

private:
    int epfd;
    struct epoll_event event {};
};

class IoQueue
{
    using SockfdQueueType = std::queue <SockEpollPtr *>;
public:
    enum QUEUE_STATUS
    {
        STATUS_RECV,
        STATUS_SEND
    };
    inline SockfdQueueType &sockfdQueue () noexcept
    {
        return sockfdQueue_;
    }
    inline void setStatus (QUEUE_STATUS status) noexcept
    {
        status_ = status;
    }
    inline std::vector <std::string> &recvVec () noexcept
    {
        return recvVec_;
    }
    inline std::vector <std::string> &sendVec () noexcept
    {
        return sendVec_;
    }

    void ioHandler ()
    {
        switch (status_)
        {
            case STATUS_RECV:
                recvHandler();
                break;
            case STATUS_SEND:
                break;
        }
    }
private:
    void recvHandler ()
    {
        SockEpollPtr *sockParams;
        int ret;
        char message[MESSAGE_SIZE_MAX];
        while (!sockfdQueue_.empty())
        {
            sockParams = sockfdQueue_.front();
            sockfdQueue_.pop();

            do
            {
                ret = recv(sockParams->fd, message, MESSAGE_SIZE_MAX, 0);
                // 不做处理  留给mainReactor去处理
                if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    PRINT_ERROR("recv error : %s", std::strerror(errno));
                    break;
                }
                else if (ret == 0)
                {
                    PRINT_INFO("pipe close : %s, sockfd : %d",
                        Utils::getIpAndHost(sockParams->sockaddr).c_str(),
                        sockParams->fd);
                    break;
                }
            } while (ret <= 0);

            recvVec_.emplace_back(message);
        }
    }

    SockfdQueueType sockfdQueue_;
    QUEUE_STATUS status_ = STATUS_RECV;
    std::vector <std::string> recvVec_;
    std::vector <std::string> sendVec_;
};
class IoReactor : Reactor, public IoQueue
{
public:
    IoReactor ()
        : Reactor(
        std::bind(&IoReactor::acceptCallback, this, std::placeholders::_1),
        std::bind(&IoReactor::recvCallback, this, std::placeholders::_1),
        std::bind(&IoReactor::sendCallback, this, std::placeholders::_1)), IoQueue() {}

private:
    void acceptCallback (ParamsType &fnParams)
    {
    }
    void recvCallback (ParamsType &fnParams)
    {

    }
    void sendCallback (ParamsType &fnParams)
    {

    }

};

template <int IoThreadNum>
class MainReactor : Reactor, EventPoll, IoQueue
{
public:
    explicit MainReactor (
        int listenfd,
        std::chrono::milliseconds expireTime = std::chrono::milliseconds { 0 }
    )
        : Reactor(
        std::bind(&MainReactor::acceptCallback, this, std::placeholders::_1),
        std::bind(&MainReactor::recvCallback, this, std::placeholders::_1),
        std::bind(&MainReactor::sendCallback, this, std::placeholders::_1)),
          EventPoll(), params(expireTime), listenfd(listenfd), IoQueue() {}

    void mainLoop ()
    {
        auto *epollPtr = new SockEpollPtr(listenfd);
        epollAddEvent(*epollPtr);
        int ret;
        struct epoll_event *event;

        mutex.lock();
        while (!terminate)
        {
            ret = epollWait(params);

            for (int i = 0; i < ret; ++i)
            {
                event = &params.sockEvents.epollEvents[i];
                epollPtr = static_cast<SockEpollPtr *>(event->data.ptr);
                if (epollPtr->fd == listenfd)
                    accept(*epollPtr);
                else if (event->events & EPOLLIN)
                    recv(*epollPtr);
                else if (event->events & EPOLLOUT)
                    send(*epollPtr);
                else if (event->events & EPOLLRDHUP)
                {
                    PRINT_INFO("对端关闭 : %d\n", event->data.fd);
                    closeSock(*epollPtr);
                }
                else if (event->events & EPOLLHUP)
                {
                    PRINT_WARNING("连接发生挂起 : %d\n", event->data.fd);
                    closeSock(*epollPtr);
                }
            }

            distributeRecvSocket();
        }
    }
private:
    void acceptCallback (ParamsType &fnParams)
    {
        static socklen_t len = sizeof(struct sockaddr);
        int fd = ::accept(listenfd, (struct sockaddr *)&clientAddr, &len);
        if (fd == -1)
        {
            PRINT_ERROR("accept error : %s\n", std::strerror(errno));
            return;
        }

        auto *sockParams = new SockEpollPtr(fd);
        epollAddEvent(*sockParams);

        PRINT_INFO("accept by %s:%d , fd : %d",
            inet_ntoa(clientAddr.sin_addr),
            ntohs(clientAddr.sin_port),
            fd);
    }
    void recvCallback (ParamsType &fnParams)
    {
        sockfdQueue().emplace(&fnParams);
    }
    void sendCallback (ParamsType &fnParams)
    {

    }

    void distributeRecvSocket ()
    {
        recvCount = sockfdQueue().size();
        int size = std::ceil(sockfdQueue().size() / (IoThreadNum + 1));

        SockEpollPtr *sockfd;
        IoReactor *io = &ioReactor[0];
        while (sockfdQueue().size() > size)
        {
            sockfd = sockfdQueue().front();
            sockfdQueue().pop();

            io->sockfdQueue().emplace(sockfd);
            if (++io == reinterpret_cast<IoReactor *>(&ioReactor + 1))
                io = &ioReactor[0];
        }

        setStatus(STATUS_RECV);
        io->setStatus(STATUS_RECV);

        mutex.unlock();
    }

    inline void closeSock (ParamsType &fnParams) noexcept
    {
        epollDelEvent(fnParams);
        close(fnParams.fd);
        delete &fnParams;
    }

private:
    bool terminate = false;
    ReactorParams params;
    int listenfd;
    struct sockaddr_in clientAddr {};
    std::mutex mutex;
    int recvCount = 0;

    IoReactor ioReactor[IoThreadNum];
};
#endif //LINUX_SERVER_LIB_KV_STORE_EPOLL_H_
