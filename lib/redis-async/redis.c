//
// Created by 115282 on 2023/7/18.
//
#include "redis-async.h"

static void redisReadHandler(int fd, int events, void *privdata) {

}

static void redisWriteHandler(int fd, int events, void *privdata) {

}
static void redisAddRead (void *privdata) {
    EpollEvent *event = (EpollEvent *)privdata;

}
int redisAttach (Reactor *r, redisAsyncContext *ac)
{
    redisContext *context = &ac->c;
    EpollEvent *event = hi_malloc(sizeof(EpollEvent));

    if (!event)
        return REDIS_ERR;

    event->reactor = r;
    event->asyncContext = ac;
    event->clientfd = context->fd;

    ac->ev.data = event;
    return REDIS_OK;
}