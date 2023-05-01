//
// Created by Administrator on 2023/3/15.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MAX_BUFFER        128
#define MAX_EPOLLSIZE    (384*1024)
#define MAX_PORT        1

int isContinue = 0;

static int ntySetNonblock (int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) return -1;
    return 0;
}

static int ntySetReUseAddr (int fd)
{
    int reuse = 1;
    return setsockopt(
        fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char *)&reuse,
        sizeof(reuse));
}
int main (int argc, char **argv)
{
    if (argc <= 2)
    {
        printf("Usage: %s ip port\n", argv[0]);
        exit(0);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int connections = 0;
    char buffer[MAX_BUFFER] = { 0 };
    int i = 0, index = 0;

    struct epoll_event events[MAX_EPOLLSIZE];

    int epoll_fd = epoll_create(MAX_EPOLLSIZE);

    strcpy(buffer, " Data From MulClient\n");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);

    while (1)
    {
        if (++index >= MAX_PORT) index = 0;
        struct epoll_event ev;
        int sockfd = 0;

        if (connections < 340000 && !isContinue)
        {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1)
            {
                perror("socket");
                goto err;
            }

            //ntySetReUseAddr(sockfd);
            addr.sin_port = htons(port + index);

            if (connect(
                sockfd,
                (struct sockaddr *)&addr,
                sizeof(struct sockaddr_in)) < 0)
            {
                perror("connect");
                goto err;
            }
            ntySetNonblock(sockfd);
            ntySetReUseAddr(sockfd);

            sprintf(buffer, "Hello Server: client --> %d\n", connections);
            send(sockfd, buffer, strlen(buffer), 0);

            ev.data.fd = sockfd;
            ev.events = EPOLLIN;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

            // char rBuffer[MAX_BUFFER] = { 0 };
            // ssize_t length = recv(sockfd, rBuffer, MAX_BUFFER, 0);
            // printf("length: %zd\n", length);
            // if (length > 0)
            // {
            //     printf(" RecvBuffer:%s\n", rBuffer);
            //
            //     if (!strcmp(rBuffer, "quit"))
            //     {
            //         isContinue = 0;
            //     }
            //
            // }
            connections++;
        }
        //connections ++;
        // if (connections % 100 == 99 || connections >= 340000)
        // {
        printf("connections: %d, sockfd:%d", connections, sockfd);

        int nfds = epoll_wait(epoll_fd, events, connections, 100);
        for (i = 0; i < nfds; i++)
        {
            int clientfd = events[i].data.fd;

            if (events[i].events & EPOLLIN)
            {
                char rBuffer[MAX_BUFFER] = { 0 };
                ssize_t length = recv(sockfd, rBuffer, MAX_BUFFER, 0);
                if (length > 0)
                {
                    printf(" RecvBuffer:%s\n", rBuffer);

                    if (!strcmp(rBuffer, "quit"))
                    {
                        isContinue = 0;
                    }

                }
                else if (length == 0)
                {
                    printf(" Disconnect clientfd:%d\n", clientfd);
                    connections--;
                    close(clientfd);
                }
                else
                {
                    if (errno == EINTR) continue;
                    if (errno == EWOULDBLOCK && errno == EAGAIN)
                    {
                        i--;
                        continue;
                    }

                    printf(
                        " Error clientfd:%d, errno:%d\n",
                        clientfd,
                        errno);
                    close(clientfd);
                }
            }
            else
            {
                printf(" clientfd:%d, errno:%d\n", clientfd, errno);
                close(clientfd);
            }
        }
        // }

        usleep(500);
    }

    return 0;

err:
    printf("error : %s\n", strerror(errno));
    return 0;
}