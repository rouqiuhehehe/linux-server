//
// Created by 115282 on 2023/7/13.
//

#include "redis-async.h"

Reactor *reactorCreate ()
{
    Reactor *reactor = malloc(sizeof(Reactor));
    reactor->epfd = epoll_create(1024);
    reactor->listenfd = 0;
    reactor->stop = 0;
    reactor->recvFn = NULL;
    reactor->acceptFn = NULL;
    reactor->sendFn = NULL;
    memset(reactor->epollEvent, 0, sizeof(struct epoll_event) * MAX_EPOLL_NUM);

    return reactor;
}
void reactorDestroy (Reactor *reactor)
{
    close(reactor->epfd);
    free(reactor);
}

static void __reactorOnceLoop (Reactor *reactor)
{
    int ret = epoll_wait(reactor->epfd, reactor->epollEvent, MAX_EPOLL_NUM, -1);
    struct epoll_event *ev;
    EpollEvent *event;
    for (int i = 0; i < ret; ++i)
    {
        ev = &reactor->epollEvent[i];
        event = ev->data.ptr;
        if (ev->events & EPOLLIN)
        {
            // 作为客户端连接redis 不存在accept
            if (event->clientfd == reactor->listenfd)
                reactor->acceptFn(event, ev->events);
            else
                reactor->recvFn(event, ev->events);
        }
        else if (ev->events & EPOLLOUT)
            reactor->sendFn(event, ev->events);
        else if (ev->events & EPOLLHUP || ev->events & EPOLLERR)
        {
            //
        }
    }
}

void reactorRun (Reactor *reactor)
{
    while (!reactor->stop)
        __reactorOnceLoop(reactor);
}
void reactorStop (Reactor *reactor)
{
    reactor->stop = 1;
}