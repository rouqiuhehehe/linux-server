//
// Created by 115282 on 2023/9/19.
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>

#define CHECK_RET(expr, fnName)                                                 \
            if (expr)                                                           \
            {                                                                   \
                fprintf(stderr,"%s:%s:%d:  "#fnName" err %s\n",                 \
                         __FILE__, __FUNCTION__, __LINE__, strerror(errno));    \
                exit(EXIT_FAILURE);                                             \
            }

#define PORT 3000
#define ADDR INADDR_ANY
#define MESSAGE_SIZE_MAX 65535

int main ()
{
    setbuf(stdout, 0);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(sockfd < 3, socket);

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int));

    struct sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = {
            .s_addr = ADDR
        }
    };
    CHECK_RET(bind(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0, bind);
    CHECK_RET(listen(sockfd, 1024) != 0, listen);
    printf("create server on listen : %d", PORT);

    sockaddr sockAddr {};
    socklen_t len = sizeof(sockaddr);
    int clientfd = accept(sockfd, &sockAddr, &len);

    int bufferSize = MESSAGE_SIZE_MAX;
    socklen_t bufferSizeLen = sizeof(bufferSize);
    setsockopt(clientfd, SOL_SOCKET, SO_RCVBUF, &bufferSize, bufferSizeLen);
    char message[MESSAGE_SIZE_MAX];
    while (1)
    {
        int ret = recv(clientfd, message, MESSAGE_SIZE_MAX, 0);
    }

    return 0;
}