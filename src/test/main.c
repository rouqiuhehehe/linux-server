//
// Created by Administrator on 2023/3/28.
//

#define _GNU_SOURCE
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <dlfcn.h>

typedef int (*socket_t) (int, int, int);
int socket (int domain, int type, int protocol)
{
    setbuf(stdout, NULL);
    socket_t socket_f = dlsym(RTLD_NEXT, "socket");
    printf("调用socket\n");

    return socket_f(domain, type, protocol);
}

int main ()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    void *stack;
    posix_memalign(&stack, getpagesize(), 128 * 1024);
    printf("%d\n", getpagesize());
    char a = 0;
    char b = 0;
    char c = 0;

    int aa = 1;
    int *bb = &aa;
    long *cc = &aa;
    return 0;
}