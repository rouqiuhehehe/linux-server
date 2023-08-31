//
// Created by Yoshiki on 2023/8/7.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_TCP_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_TCP_H_

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "util/global.h"
#include "kv-reactor.h"

template <int IoThreadNum = 4>
class Tcp
{
public:
    explicit Tcp (uint16_t port)
        : sockfd(createTcpServer(port, INADDR_ANY)), reactor(sockfd) {}
    Tcp (uint16_t port, const std::string &host) // NOLINT
        : sockfd(createTcpServer(port, inet_addr(host.c_str()))), reactor(sockfd) {}

    void mainLoop ()
    {
        reactor.mainLoop();
    }

private:
    static int createTcpServer (uint16_t port, in_addr_t inAddr)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        CHECK_RET(sockfd < 3, socket);
        MainReactor <IoThreadNum>::setSockReuseAddr(sockfd);
        
        struct sockaddr_in addr {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {
                .s_addr = inAddr
            }
        };
        CHECK_RET(bind(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0, bind);
        CHECK_RET(listen(sockfd, 1024) != 0, listen);

        PRINT_INFO("create server on listen : %d", port);

        return sockfd;
    }

    int sockfd;
    MainReactor <IoThreadNum> reactor;
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_TCP_H_
