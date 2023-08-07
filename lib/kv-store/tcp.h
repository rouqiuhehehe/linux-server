//
// Created by Yoshiki on 2023/8/7.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_TCP_H_
#define LINUX_SERVER_LIB_KV_STORE_TCP_H_

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "global.h"
#include "reactor.h"

class Tcp
{
public:
    Tcp (const std::string &host, uint16_t port) // NOLINT
    {
        createTcpServer(host, port);
    }

private:
    void createTcpServer (const std::string &host, uint16_t port)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        CHECK_RET(sockfd < 3, socket);

        struct sockaddr_in addr {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {
                .s_addr = inet_addr(host.c_str())
            }
        };
        CHECK_RET(bind(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0, bind);
        CHECK_RET(listen(sockfd, 1024) != 0, listen);
    }

    int sockfd;
};
#endif //LINUX_SERVER_LIB_KV_STORE_TCP_H_
