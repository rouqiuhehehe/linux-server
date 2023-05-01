//
// Created by Administrator on 2023/3/30.
//
#include "coroutine.h"
int coroutineEpollCreate (void)
{
    return epoll_create(1024);
}

int coroutineEpollRegisterTrigger (void)
{
    Schedule *schedule = coroutineGetSchedule();

    if (schedule == NULL)
    {
        printf("please init schedule and then to register epoll");
        return -1;
    }

    // 创建一个文件描述符
    if (!schedule->eventfd)
    {
        schedule->eventfd = eventfd(0, EFD_NONBLOCK);
        assert(schedule->eventfd >= 3);
    }

    struct epoll_event event = {};
    event.data.fd = schedule->eventfd;
    event.events = EPOLLIN;

    assert(epoll_ctl(
        schedule->epfd,
        EPOLL_CTL_ADD,
        schedule->eventfd,
        &event
    ) != -1);

    return 0;
}

int coroutineEpollWait (struct timespec *timeval)
{
    Schedule *schedule = coroutineGetSchedule();
    return epoll_wait(
        schedule->epfd,
        schedule->eventList,
        COROUTINE_MAX_EVENTS,
        timeval->tv_sec * 1000u + timeval->tv_nsec / 1000000u
    );
}