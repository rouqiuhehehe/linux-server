//
// Created by Yoshiki on 2023/7/15.
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <wait.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"

#define WORKER_PROCESS_NUM 4
#define EPOLL_MAX_NUM 512
#define BUFFER_LEN (1024 * 64)
#define MESSAGE "server get your message"

typedef struct
{
    pthread_spinlock_t spinlock;
    volatile int terminate;
} ShareData;

int shmId;
ShareData *shareData;
void shareMemoryDestroy ();
int createTcpServer (uint32_t port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(sockfd < 3, socket);

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    CHECK_RET(bind(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr)), bind);
    CHECK_RET(listen(sockfd, 10), listen);

    SET_FD_NONBLOCK(sockfd);
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));

    return sockfd;
}

void workProcess (int sockfd)
{
    int epfd = epoll_create(1);

    struct epoll_event ev, events[EPOLL_MAX_NUM];
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    ssize_t nready;
    char buffer[BUFFER_LEN];
    struct sockaddr addr;
    socklen_t len = sizeof(struct sockaddr);

    for (;;)
    {
        if (shareData->terminate)
        {
            shareMemoryDestroy();
            exit(EXIT_SUCCESS);
        }

        int ret = pthread_spin_trylock(&shareData->spinlock);
        if (ret == 0)
        {
            nready = epoll_wait(epfd, events, EPOLL_MAX_NUM, 10);
            for (int i = 0; i < nready; ++i)
            {
                if (events[i].events & EPOLLIN)
                {
                    if (events[i].data.fd == sockfd)
                    {
                        int connectfd = accept(sockfd, &addr, &len);
                        printf("accept by process : %d\n", getpid());
                        if (connectfd < 0)
                            continue;
                        ev.data.fd = connectfd;
                        ev.events = EPOLLIN;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, connectfd, &ev);
                    }
                    else
                    {
                        nready = recv(events[i].data.fd, buffer, BUFFER_LEN, 0);
                        if (nready == 0)
                        {
                            ev.data.fd = events[i].data.fd;
                            ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                            close(events[i].data.fd);
                        }
                        else if (nready < 0)
                            continue;
                        else
                        {
                            printf(
                                "fd : %u, recv : %s\n",
                                events[i].data.fd,
                                buffer
                            );
                            ev.data.fd = events[i].data.fd;
                            ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                        }
                    }
                }
                else if (events[i].events & EPOLLOUT)
                {
                    nready = send(events[i].data.fd, MESSAGE, strlen(MESSAGE), 0);
                    if (nready == 0)
                    {
                        ev.data.fd = events[i].data.fd;
                        ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                        close(events[i].data.fd);
                    }
                    else if (nready < 0)
                        continue;
                    else
                    {
                        ev.data.fd = events[i].data.fd;
                        ev.events = EPOLLIN;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                    }
                }
            }

            pthread_spin_unlock(&shareData->spinlock);
        }
    }
}

int sharedMemoryInit ()
{
    key_t key = ftok(".", 1);
    CHECK_RET(key < 0, frok);

    shmId = shmget(key, sizeof(ShareData), IPC_CREAT | 0644);
    CHECK_RET(shmId < 0, shmget);

    // 将共享内存连接到当前进程的地址空间
    shareData = shmat(shmId, NULL, 0);
    CHECK_RET(shareData == (void *)-1, shmat);

    return shmId;
}

void shareMemoryDestroy ()
{
    shmdt(shareData);
    shmctl(shmId, IPC_RMID, NULL);
}
void exitMasterProcess ()
{
    shareData->terminate = 1;
    for (int i = 0; i < WORKER_PROCESS_NUM; ++i)
    {
        int status;
        wait(&status);
        if (WIFEXITED(status))
        {
            printf("work process exit : %d\n", WEXITSTATUS(status));
        }
    }
    shareMemoryDestroy();
    exit(EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
    setbuf(stdout, 0);

    printf("%d\n", getpid());
    if (argc < 2)
    {
        fprintf(stdout, "lose port params");
        exit(EXIT_FAILURE);
    }
    long port = strtol(argv[1], NULL, 10);
    int sockfd = createTcpServer(port);

    sharedMemoryInit();
    shareData->terminate = 0;
    pthread_spin_init(&shareData->spinlock, PTHREAD_PROCESS_SHARED);

    int pid;

    for (int i = 0; i < WORKER_PROCESS_NUM; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            perror("some thing error");
            exitMasterProcess();
        }
        if (pid == 0)
            workProcess(sockfd);
    }

    signal(SIGINT, exitMasterProcess);

    for (;;)
    {
        // sleep(1);
    }
}