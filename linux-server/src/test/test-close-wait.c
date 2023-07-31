//
// Created by 115282 on 2023/7/17.
//

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "global.h"

#define EPOLL_MAX_EVENTS 512
#define BUFFER_LEN (1024 * 64)

int main ()
{
    setbuf(stdout, 0);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(sockfd < 3, socket);

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(3000);
    addr.sin_family = AF_INET;

    CHECK_RET(bind(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0, bind);
    CHECK_RET(listen(sockfd, 10) != 0, listen);

    int epfd = epoll_create(1024);
    CHECK_RET(epfd < 3, epoll_create);

    struct epoll_event ev, events[EPOLL_MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    int nready, clientfd;
    ssize_t bufferLen;
    char buffer[BUFFER_LEN];
    struct epoll_event *event;
    struct sockaddr ad;
    socklen_t len = sizeof(struct sockaddr);

    for (;;)
    {
        nready = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, -1);
        for (int i = 0; i < nready; ++i)
        {
            event = &events[i];
            if (event->events & EPOLLRDHUP)
            {
                printf("ddddddddddddddddd\n");
            }
            else if (event->events & EPOLLIN)
            {
                if (event->data.fd == sockfd)
                {
                    clientfd = accept(sockfd, &ad, &len);
                    if (clientfd < 3) continue;

                    ev.data.fd = clientfd;
                    ev.events = EPOLLIN | EPOLLRDHUP;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                }
                else
                {
                    bufferLen = recv(event->data.fd, buffer, BUFFER_LEN, 0);
                    if (bufferLen < 0) continue;

                    if (bufferLen == 0)
                    {
                        // 先不关闭
                    }
                    else
                    {
                        printf("recv : %s\n", buffer);
                    }

                }
            }
        }
    }
}