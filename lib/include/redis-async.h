//
// Created by 115282 on 2023/7/13.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_REDIS_ASYNC_H_
#define LINUX_SERVER_LIB_INCLUDE_REDIS_ASYNC_H_

#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <hiredis/async.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_EPOLL_NUM 1024

typedef struct __e EpollEvent;
typedef void (*ReactorCallback) (EpollEvent *privdata, uint32_t events);
typedef struct
{
    int epfd;
    int listenfd;
    int stop;
    ReactorCallback acceptFn;
    ReactorCallback recvFn;
    ReactorCallback sendFn;
    struct epoll_event epollEvent[MAX_EPOLL_NUM];
} Reactor;

typedef struct __e
{
    Reactor *reactor;
    int clientfd;
    redisAsyncContext *asyncContext;
} EpollEvent;

Reactor *reactorCreate ();
void reactorDestroy (Reactor *);

void reactorRun (Reactor *);
void reactorStop (Reactor *);

int redisAttach (Reactor *r, redisAsyncContext *ac);
#ifdef __cplusplus
};
#endif
#endif //LINUX_SERVER_LIB_INCLUDE_REDIS_ASYNC_H_
