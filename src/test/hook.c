//
// Created by Administrator on 2023/3/28.
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <sys/socket.h>
#include <stdio.h>

typedef int (*socket_t) (int, int, int);
int socket (int domain, int type, int protocol)
{
    setbuf(stdout, NULL);
    socket_t socket_f = dlsym(RTLD_NEXT, "socket");
    printf("调用socket\n");

    return socket_f(domain, type, protocol);
}