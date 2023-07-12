//
// Created by Administrator on 2023/4/3.
//

#include "coroutine.h"
#include <arpa/inet.h>

#define PORT 3000
#define MAX_LISTEN_PORT 100

#define TIME_USE(t1, t2) (((t1).tv_sec - (t2).tv_sec) * 1000 + ((t1).tv_usec - (t2).tv_usec) / 1000)

void serverReader (void *arg)
{
    int fd = *(int *)arg;
    free(arg);
    char buf[1024] = {};
    int ret;
    while (1)
    {
        ret = recv(fd, buf, 1024, 0);
        if (ret > 0)
        {
            ret = send(fd, buf, 1024, 0);

            if (ret == 0)
            {
                close(fd);
                break;
            }
        }
        else if (ret == 0)
        {
            close(fd);
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            break;
        }
    }
}
_Noreturn void server (void *arg)
{
    uint16_t port = *(uint16_t *)arg;
    free(arg);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 3)
        return;

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in sockaddrIn = {};
    struct sockaddr_in sockaddrClient = {};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_addr.s_addr = INADDR_ANY;
    sockaddrIn.sin_port = htons(port);

    if (bind(
        sockfd,
        (const struct sockaddr *)&sockaddrIn,
        sizeof(struct sockaddr)) != 0)
        return;

    if (listen(sockfd, 20) != 0)
        return;

    printf("server listen on port : %d", port);

    struct timeval begin;
    gettimeofday(&begin, 0);

    while (1)
    {
        socklen_t len = sizeof(struct sockaddr);

        int clientfd = accept(sockfd, (struct sockaddr *)&sockaddrClient, &len);

        if (clientfd % 1000 == 999)
        {
            struct timeval end;
            gettimeofday(&end, 0);

            int timeUse = TIME_USE(end, begin);
            memcpy(&begin, &end, sizeof(struct timeval));

            printf("client fd: %d, timeUse: %d\n", clientfd, timeUse);
        }

        Coroutine *readCoroutine = NULL;
        int *fd = calloc(0, sizeof(int));
        *fd = clientfd;
        coroutineCreate(&readCoroutine, serverReader, fd);
    }
}

int main ()
{
    setbuf(stdout, NULL);
    initHook();
    Coroutine *coroutine = NULL;
    for (int i = 0; i < MAX_LISTEN_PORT; ++i)
    {
        uint16_t *port = calloc(0, sizeof(uint16_t));
        *port = PORT + i;

        coroutineCreate(&coroutine, server, port);
    }

    // 此处开始执行server
    coroutineScheduleRun();
}