//
// Created by 115282 on 2023/7/18.
//
#include "redis-async.h"
#include "global.h"

#define EPOLL_EVENT_CTL(__event, __events, op)                                      \
    struct epoll_event ev;                                                          \
    (ev).events = (__events);                                                       \
    (ev).data.ptr = event;                                                          \
    int ret = epoll_ctl((__event)->reactor->epfd, op, (__event)->clientfd, &(ev));  \
    CHECK_RET(ret == -1, epoll_ctl)

#define EPOLL_EVENT_ADD(event, events) EPOLL_EVENT_CTL(event, events, EPOLL_CTL_ADD)
#define EPOLL_EVENT_DEL(event)          \
    if (!(event)->mask) return;         \
    (event)->mask = 0;                  \
    EPOLL_EVENT_CTL(event, 0, EPOLL_CTL_DEL)

#define EPOLL_EVENT_MOD(event, events) EPOLL_EVENT_CTL(event, events, EPOLL_CTL_MOD)
#define EPOLL_EVENT_HDL(event, events, isRemove)    \
    if ((event)->mask & (events)) return;           \
    int flag;                                       \
    if ((event)->mask) flag = EPOLL_CTL_MOD;        \
    else flag = EPOLL_CTL_ADD;                      \
    if (isRemove) (event)->mask &= ~(events);       \
    else (event)->mask |= (events);                 \
    EPOLL_EVENT_CTL(event, (event)->mask, flag)

static void redisReadHandler (EpollEvent *event, __attribute__((unused)) uint32_t events)
{
    redisAsyncHandleRead(event->asyncContext);
}

static void redisWriteHandler (EpollEvent *event, __attribute__((unused)) uint32_t events)
{
    redisAsyncHandleWrite(event->asyncContext);
}
static void redisAddRead (void *privdata)
{
    EpollEvent *event = (EpollEvent *)privdata;
    EPOLL_EVENT_HDL(event, EPOLLIN, 0);
}
static void redisDelRead (void *privdata)
{
    EpollEvent *event = (EpollEvent *)privdata;
    EPOLL_EVENT_HDL(event, EPOLLIN, 1);
}
static void redisAddWrite (void *privdata)
{
    EpollEvent *event = (EpollEvent *)privdata;
    EPOLL_EVENT_HDL(event, EPOLLOUT, 0);
}
static void redisDelWrite (void *privdata)
{
    EpollEvent *event = (EpollEvent *)privdata;
    EPOLL_EVENT_HDL(event, EPOLLOUT, 1);
}
static void redisClean (void *privdata)
{
    EpollEvent *event = (EpollEvent *)privdata;
    EPOLL_EVENT_DEL(event);
    hi_free(privdata);
}
int redisAttach (Reactor *r, redisAsyncContext *ac)
{
    redisContext *context = &ac->c;
    EpollEvent *event = hi_malloc(sizeof(EpollEvent));

    if (!event)
        return REDIS_ERR;

    r->recvFn = redisReadHandler;
    r->sendFn = redisWriteHandler;

    event->reactor = r;
    event->asyncContext = ac;
    event->clientfd = context->fd;
    event->mask = 0;

    ac->ev.data = event;
    ac->ev.addRead = redisAddRead;
    ac->ev.addWrite = redisAddWrite;
    ac->ev.delRead = redisDelRead;
    ac->ev.delWrite = redisDelWrite;
    ac->ev.cleanup = redisClean;
    ac->ev.scheduleTimer = NULL;

    return REDIS_OK;
}