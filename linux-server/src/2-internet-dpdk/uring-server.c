//
// Created by 115282 on 2023/5/25.
// 需要22以上版本的Ubuntu才能跑
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <liburing.h>

#include <time.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
#define ENTRIES_LENGTH 1024
#define BUFFER_LENGTH 1024
#define CQES_LEN 100

#define CHECK_RET(exp, funName, retNum) {  \
    if (exp) {                             \
        perror(#funName" error\n");        \
        return retNum;                     \
    }                                      \
}
#define MAIN_LOOP_CHECK_RET(exp, ret, funName) {  \
    if (exp) {                                    \
        errno = -ret;                             \
        perror(#funName" error\n");               \
        goto error;                               \
    }                                             \
}

enum
{
    EVENT_ACCEPT,
    EVENT_READ,
    EVENT_WRITE
};
typedef struct
{
    int events;
    int clientfd;
} ClientInfo;
static char STATUS = 0;

static int createTcpServerListen (uint32_t port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));
    CHECK_RET(sockfd < 3, socket, -1)

    struct sockaddr_in sockaddrIn = {};
    sockaddrIn.sin_addr.s_addr = INADDR_ANY;
    sockaddrIn.sin_port = htons(port);
    sockaddrIn.sin_family = AF_INET;

    CHECK_RET(
        bind(sockfd, (const struct sockaddr *)&sockaddrIn, sizeof(struct sockaddr)) != 0,
        bind,
        -2
    )

    CHECK_RET(listen(sockfd, 5) != 0, listen, -3)

    printf("server listening on %d\n", port);
    return sockfd;
}

static void setAcceptEvent (
    struct io_uring *ring,
    int sockfd,
    struct sockaddr *sockaddr,
    socklen_t *len,
    int flags
)
{
    // submission queue entry
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    // 初始化sqe，准备接受异步accept
    io_uring_prep_accept(sqe, sockfd, sockaddr, len, flags);
    ClientInfo info = {
        .events = EVENT_ACCEPT,
        .clientfd = sockfd
    };

    memcpy(&sqe->user_data, &info, sizeof(ClientInfo));
}

static void setRecvEvent (
    struct io_uring *ring,
    int sockfd,
    char *buffer,
    int len,
    int flags
)
{
    // submission queue entry
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, sockfd, buffer, len, flags);
    ClientInfo info = {
        .events = EVENT_READ,
        .clientfd = sockfd
    };

    memcpy(&sqe->user_data, &info, sizeof(ClientInfo));
}

static void setSendEvent (
    struct io_uring *ring,
    int sockfd,
    const char *buffer,
    int len,
    int flags
)
{
    // submission queue entry
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, sockfd, buffer, len, flags);
    ClientInfo info = {
        .events = EVENT_WRITE,
        .clientfd = sockfd
    };

    memcpy(&sqe->user_data, &info, sizeof(ClientInfo));
}

static void mainLoop (int sockfd)
{
    struct io_uring_params params = {};
    struct io_uring ring = {};

    io_uring_queue_init_params(ENTRIES_LENGTH, &ring, &params);

    struct sockaddr sockaddr = {};
    socklen_t len = sizeof(sockaddr);

    setAcceptEvent(&ring, sockfd, &sockaddr, &len, 0);

    char buffer[BUFFER_LENGTH] = {};
    // completion queue entry
    struct io_uring_cqe *cqe, *cqes[CQES_LEN];
    int ret, resume = 1;
    int sum = 0;
    unsigned count;
    ClientInfo clientInfo = {};
    time_t begin;
    time_t end;
    while (1)
    {
        // 提交一个事件
        ret = io_uring_submit(&ring);
        MAIN_LOOP_CHECK_RET(ret < 0, ret, io_uring_submit)

        // 等待提交的事件完成
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret == -EINTR)
            continue;
        MAIN_LOOP_CHECK_RET(ret < 0, ret, io_uring_wait_cqe)

        if (uring_unlikely(STATUS == 0))
        {
            STATUS = 1;
            begin = time(NULL);
        }

        count = io_uring_peek_batch_cqe(&ring, cqes, CQES_LEN);
        for (int i = 0; i < count; ++i)
        {
            cqe = cqes[i];
            memcpy(&clientInfo, &cqe->user_data, sizeof(ClientInfo));

            if (clientInfo.events == EVENT_ACCEPT)
            {
                // clientfd
                // printf("clientfd : %d - ------\n", cqe->res);
                MAIN_LOOP_CHECK_RET(cqe->res < 3, -EBADF, cqe->res)

                setsockopt(cqe->res, SOL_SOCKET, SO_REUSEADDR, (const char *)&resume, sizeof(int));
                setAcceptEvent(&ring, sockfd, &sockaddr, &len, 0);
                setRecvEvent(&ring, cqe->res, buffer, BUFFER_LENGTH, 0);
            }
            else if (clientInfo.events == EVENT_READ)
            {
                if (cqe->res == -ECONNRESET)
                {
                    close(clientInfo.clientfd);
                    continue;
                }
                MAIN_LOOP_CHECK_RET(cqe->res < 0, cqe->res, recv)

                if (cqe->res == 0)
                {
                    close(clientInfo.clientfd);
                    continue;
                }

                setSendEvent(&ring, clientInfo.clientfd, buffer, cqe->res, 0);
            }
            else if (clientInfo.events == EVENT_WRITE)
            {
                if (cqe->res == -ECONNRESET)
                {
                    close(clientInfo.clientfd);
                    continue;
                }
                MAIN_LOOP_CHECK_RET(cqe->res < 0, cqe->res, send)

                if (cqe->res == 0)
                {
                    close(clientInfo.clientfd);
                    continue;
                }

                setRecvEvent(&ring, clientInfo.clientfd, buffer, BUFFER_LENGTH, 0);
            }

            sum++;
            if (uring_unlikely(sum % 9999 == 0))
            {
                end = time(NULL);
                printf("connect : %d, time use : %ld\n", sum, end - begin);
                begin = time(NULL);
            }
        }

        // 将属于ring的count个io完成事件标记为已使用
        io_uring_cq_advance(&ring, count);
    }

error :
    close(sockfd);
    exit(EXIT_FAILURE);
}

int main (int argc, char **argv)
{
    setbuf(stdout, 0);
    if (argc != 2)
    {
        errno = EINVAL;
        perror("argument error [port]\n");
        goto error;
    }

    uint32_t port = strtol(argv[1], NULL, 10);
    int sockfd = createTcpServerListen(port);
    if (sockfd < 3)
        goto error;

    mainLoop(sockfd);
    return 0;

error :
    exit(EXIT_FAILURE);
}
#pragma clang diagnostic pop