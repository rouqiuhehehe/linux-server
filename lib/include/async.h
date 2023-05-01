//
// Created by Administrator on 2023/3/27.
//

#ifndef LINUX_SERVER_LIB_ASYNC_H_
#define LINUX_SERVER_LIB_ASYNC_H_
#include <sys/epoll.h>

struct AsyncInfo;
typedef void (*Callback) (struct AsyncInfo *, struct epoll_event *);
struct AsyncInfo
{
    Callback cb;
    int epfd;
    pthread_t pid;
    void *arg;
};

int asyncIOInit (struct AsyncInfo *info, Callback cb, void *arg);
int asyncIODestroy (struct AsyncInfo **info);
#endif //LINUX_SERVER_LIB_ASYNC_H_
