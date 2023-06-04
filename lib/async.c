//
// Created by Administrator on 2023/3/27.
//

#include "include/async.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define EPOLL_EVENT_LEN 1024

void *callback (void *arg)
{
    struct AsyncInfo *info = (struct AsyncInfo *)arg;
    struct epoll_event epollEvent[EPOLL_EVENT_LEN] = {};
    while (1)
    {
        int nready = epoll_wait(info->epfd, epollEvent, EPOLL_EVENT_LEN, -1);

        for (int i = 0; i < nready; ++i)
            info->cb(info, &epollEvent[i]);
    }
}

int asyncIOInit (struct AsyncInfo *info, Callback cb, void *arg)
{
    if (!cb)
        return -1;

    memset(info, 0, sizeof(struct AsyncInfo));
    info->epfd = epoll_create(1);
    info->cb = cb;
    info->arg = arg;

    if (pthread_create(&info->pid, NULL, callback, info))
    {
        perror("pthread_create error");
        return -3;
    }
    usleep(1);

    return 0;
}

int asyncIODestroy (struct AsyncInfo **info)
{
    pthread_cancel((*info)->pid);
    close((*info)->epfd);
    free(*info);
    *info = NULL;
    return 0;
}