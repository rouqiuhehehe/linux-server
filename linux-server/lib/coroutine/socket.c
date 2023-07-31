//
// Created by Administrator on 2023/4/2.
//
#include "coroutine.h"

#ifdef COROUTINE_HOOK

extern void coroutineScheduleWait (Coroutine *coroutine,
    int fd,
    unsigned short events,
    uint64_t timeout);
extern void coroutineYield (Coroutine *coroutine);
extern Coroutine *coroutineScheduleDeWait (int fd);

#define CHECK_HOOK() { \
    if (!socket_f || !connect_f || /**!read_f ||**/ !recv_f || !recvfrom_f \
       || /**!write_f ||**/ !send_f || !sendto_f || !accept_f || !close_f) \
       initHook();     \
}
#define SET_NONBLOCK(fd) { \
    int flag = fcntl(fd, F_GETFL); \
    flag |= O_NONBLOCK;    \
    fcntl(fd, F_SETFL, flag);      \
}
#define SET_SOCKET_REUSEADDR(fd) { \
    int reuse = 1;                 \
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); \
}
#define SET_EPOLL_EVENTS(event, sockfd) { \
    (event).events = EPOLLIN | EPOLLOUT | EPOLLHUP; \
    (event).data.fd = sockfd;               \
}

typedef int (*socket_t) (int domain, int type, int protocol);
socket_t socket_f;

typedef int(*connect_t) (int, const struct sockaddr *, socklen_t);
connect_t connect_f;

typedef ssize_t(*read_t) (int, void *, size_t);
read_t read_f;

typedef ssize_t(*recv_t) (int sockfd, void *buf, size_t len, int flags);
recv_t recv_f;

typedef ssize_t(*recvfrom_t) (int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen);
recvfrom_t recvfrom_f;

typedef ssize_t(*write_t) (int, const void *, size_t);
write_t write_f;

typedef ssize_t(*send_t) (int sockfd, const void *buf, size_t len, int flags);
send_t send_f;

typedef ssize_t(*sendto_t) (int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen);
sendto_t sendto_f;

typedef int(*accept_t) (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
accept_t accept_f;

// new-syscall
typedef int(*close_t) (int);
close_t close_f;

static int coroutineEpollInner (struct epoll_event *events, int n, int timeout)
{
    Schedule *schedule = coroutineGetSchedule();
    if (schedule == NULL)
    {
        printf("schedule not exit!\n");
        return -1;
    }
    if (timeout == 0)
        return epoll_wait(schedule->epfd, events, n, timeout);

    if (timeout < 0)
        timeout = INT32_MAX;

    Coroutine *coroutine = schedule->currThread;
    struct epoll_event event = {};
    int i = 0;
    for (; i < n; ++i)
    {
        coroutine->events = event.events = events[i].events;
        event.data.fd = events[i].data.fd;

        // 添加到EPOLL
        epoll_ctl(schedule->epfd, EPOLL_CTL_ADD, event.data.fd, &event);
        // 将当前协程设置为等待和睡眠状态
        coroutineScheduleWait(coroutine, event.data.fd, event.events, timeout);
    }

    // 让出上下文
    coroutineYield(coroutine);

    // 等到coroutineScheduleRun函数中的epoll_wait返回时
    // 唤起协程
    for (i = 0; i < n; ++i)
    {
        coroutine->events = event.events = events[i].events;
        event.data.fd = events[i].data.fd;

        epoll_ctl(schedule->epfd, EPOLL_CTL_DEL, event.data.fd, &event);
        coroutineScheduleDeWait(event.data.fd);
    }

    return n;
}

void initHook ()
{
    socket_f = (socket_t)dlsym(RTLD_NEXT, "socket");

    //read_f = (read_t)dlsym(RTLD_NEXT, "read");
    recv_f = (recv_t)dlsym(RTLD_NEXT, "recv");
    recvfrom_f = (recvfrom_t)dlsym(RTLD_NEXT, "recvfrom");

    //write_f = (write_t)dlsym(RTLD_NEXT, "write");
    send_f = (send_t)dlsym(RTLD_NEXT, "send");
    sendto_f = (sendto_t)dlsym(RTLD_NEXT, "sendto");

    accept_f = (accept_t)dlsym(RTLD_NEXT, "accept");
    close_f = (close_t)dlsym(RTLD_NEXT, "close");
    connect_f = (connect_t)dlsym(RTLD_NEXT, "connect");
}

int socket (int domain, int type, int protocol)
{
    printf("socket --------- \n");
    CHECK_HOOK();

    int fd = socket_f(domain, type, protocol);
    if (fd < 3)
    {
        printf("Failed to create new socket\n");
        return -1;
    }

    SET_NONBLOCK(fd);
    SET_SOCKET_REUSEADDR(fd);

    return fd;
}

int accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    printf("accept --------- \n");
    CHECK_HOOK();
    struct epoll_event event = {};
    int clientfd;
    while (1)
    {
        SET_EPOLL_EVENTS(event, sockfd);
        // 此处timeout设置为1，是为了防止把协程放入休眠树中
        coroutineEpollInner(&event, 1, 1);

        clientfd = accept_f(sockfd, addr, addrlen);
        if (clientfd < 0)
        {
            if (errno == EAGAIN) continue;
            else if (errno == ECONNABORTED) printf("accept : ECONNABORTED\n");
            else if (errno == EMFILE || errno == ENFILE)
                printf(
                    "accept : EMFILE || ENFILE\n"
                );
            return -1;
        }
        break;
    }

    SET_NONBLOCK(clientfd);
    SET_SOCKET_REUSEADDR(clientfd);

    return clientfd;
}

int connect (int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    printf("connect --------- \n");
    CHECK_HOOK();
    struct epoll_event event = {};
    int ret;

    while (1)
    {
        SET_EPOLL_EVENTS(event, sockfd);

        coroutineEpollInner(&event, 1, 1);

        ret = connect_f(sockfd, addr, addrlen);
        if (ret == 0) break;
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
                continue;
            break;
        }
    }

    return ret;
}

ssize_t recv (int sockfd, void *buf, size_t len, int flags)
{
    CHECK_HOOK();
    printf("recv --------- %d\n", sockfd);
    struct epoll_event event;

    SET_EPOLL_EVENTS(event, sockfd);
    coroutineEpollInner(&event, 1, 1);

    ssize_t ret = recv_f(sockfd, buf, len, flags);
    if (ret < 0)
    {
        if (errno == ECONNRESET)
            return -1;
    }
    return ret;
}

ssize_t recvfrom (int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen)
{
    printf("recvfrom --------- \n");
    struct epoll_event event;
    SET_EPOLL_EVENTS(event, sockfd);
    coroutineEpollInner(&event, 1, 1);

    ssize_t ret = recvfrom_f(sockfd, buf, len, flags, src_addr, addrlen);
    if (ret < 0)
    {
        if (errno == EAGAIN)
            return ret;
        if (errno == ECONNRESET)
            return 0;
        perror("recvfrom error \n");
    }

    return ret;
}

ssize_t send (int sockfd, const void *buf, size_t len, int flags)
{
    CHECK_HOOK();
    printf("send --------- \n");
    ssize_t sLen = 0;
    ssize_t ret = send_f(sockfd, buf, len, flags);
    if (ret == 0)
        return ret;
    if (ret > 0)
        sLen += ret;

    struct epoll_event event;
    while (sLen < len)
    {
        SET_EPOLL_EVENTS(event, sockfd);
        coroutineEpollInner(&event, 1, 1);

        ret = send_f(sockfd, (char *)buf + sLen, len - sLen, flags);
        if (ret < 0)
            break;

        sLen += ret;
    }

    if (ret <= 0 && sLen == 0)
        return ret;
    return sLen;
}

ssize_t sendto (int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen)
{
    printf("sendto --------- \n");
    struct epoll_event event;
    SET_EPOLL_EVENTS(event, sockfd);

    coroutineEpollInner(&event, 1, 1);

    ssize_t ret = sendto_f(sockfd, buf, len, flags, dest_addr, addrlen);
    if (ret < 0)
    {
        if (errno == EAGAIN)
            return ret;
        if (errno == ECONNRESET)
            return 0;

        perror("sendto error \n");
    }

    return ret;
}
#endif