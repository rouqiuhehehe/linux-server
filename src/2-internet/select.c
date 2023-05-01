//
// Created by Administrator on 2023/3/11.
//
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define CHECK_RET(ret, stdRet, fnName) {    \
            if (ret == stdRet)              \
            {                               \
                perror(#fnName"() error");  \
                exit(EXIT_FAILURE);         \
            }                               \
        }
#define BUFFER_SIZE 65535

int setFcntlNonBlock (int sockfd)
{
    int flag = fcntl(sockfd, F_GETFL);
    flag |= SOCK_NONBLOCK;
    fcntl(sockfd, F_SETFL, flag);

    return 0;
}
int createSelectTcpServer (int sockfd)
{
    fd_set rset, wset, rsetc, wsetc;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&rsetc);
    FD_ZERO(&wsetc);
    FD_SET(sockfd, &rsetc);
    int maxfd = sockfd;
    struct sockaddr_in clientSockaddr = {};
    socklen_t len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE] = {};
    int ret;

    while (1)
    {
        rset = rsetc;
        wset = wsetc;

        int nready = select(maxfd + 1, &rset, &wset, NULL, NULL);
        CHECK_RET(nready, -1, select);

        if (nready == 0) continue;
        if (FD_ISSET(sockfd, &rset))
        {
            int clientfd =
                accept(sockfd, (struct sockaddr *)&clientSockaddr, &len);
            if (clientfd == -1 && errno != EAGAIN)
            {
                CHECK_RET(clientfd, -1, accept);
            }
            FD_SET(clientfd, &rsetc);
            maxfd = clientfd;
        }
        for (int i = sockfd + 1; i < maxfd + 1; ++i)
        {
            if (FD_ISSET(i, &rset))
            {
                memset(buffer, 0, BUFFER_SIZE);
                ret = (int)recv(i, &buffer, BUFFER_SIZE, 0);
                CHECK_RET(ret, -1, read);
                if (ret == 0)
                {
                    printf("对端关闭socket\n");
                    close(i);
                    FD_CLR(i, &rsetc);
                    continue;
                }
                printf("收到客户端的消息：%s\n", buffer);
                FD_SET(i, &wsetc);
            }
            else if (FD_ISSET(i, &wset))
            {
                ret = (int)send(i, &buffer, BUFFER_SIZE, 0);
                CHECK_RET(ret, -1, send);
                FD_CLR(i, &wsetc);
                FD_SET(i, &rsetc);
            }
        }
    }
}
int main ()
{
    setbuf(stdout, NULL);
    // 从3开始，0、1、2分别是，stdin，stdout，stderr
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(sockfd, -1, socket);
    int set = 1;
    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &set,
        sizeof(int));

    struct sockaddr_in sockaddr = {};
    sockaddr.sin_port = htons(3000);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_family = AF_INET;
    int ret = bind(
        sockfd,
        (const struct sockaddr *)&sockaddr,
        sizeof(struct sockaddr));
    CHECK_RET(ret, -1, bind);

#if 0
    setFcntlNonBlock(sockfd);
#endif
    ret = listen(sockfd, 10);
    CHECK_RET(ret, -1, listen);

    createSelectTcpServer(sockfd);

    return 0;
}