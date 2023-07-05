//
// Created by 115282 on 2023/5/29.
//
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <unistd.h>

#include "shm-define.h"

int main ()
{
    setbuf(stdout, 0);
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 3)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    char buffer[BUFFER_LEN] = {};
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(int));

    if (access(SOCK_PATH, 0) == 0)
        // UNIX socket 需要绑定一个不存在的文件，如果文件存在先删除
        unlink(SOCK_PATH);

    struct sockaddr_un un = {};
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, SOCK_PATH, sizeof(un.sun_path) - 1);

    ssize_t ret = bind(sockfd, (const struct sockaddr *)&un, sizeof(un));
    if (ret == -1)
    {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    struct msghdr msgHdr = {};
    char msgBuffer[CMSG_SPACE(sizeof(int))] = {};
    msgHdr.msg_control = msgBuffer;
    msgHdr.msg_controllen = sizeof(msgBuffer);

    struct iovec iov = {};
    iov.iov_base = buffer;
    iov.iov_len = BUFFER_LEN;
    msgHdr.msg_iov = &iov;
    msgHdr.msg_iovlen = 1;

    printf("recvmsg waiting for data .... \n");

    ret = recvmsg(sockfd, &msgHdr, 0);
    if (ret == -1)
    {
        perror("recvmsg error");
        exit(EXIT_FAILURE);
    }

    struct cmsghdr *cmsghdr = CMSG_FIRSTHDR(&msgHdr);
    if (cmsghdr == NULL || cmsghdr->cmsg_len != CMSG_LEN(sizeof(int))
        || cmsghdr->cmsg_level != SOL_SOCKET || cmsghdr->cmsg_type != SCM_RIGHTS)
    {
        perror("Invalid control message");
        exit(EXIT_FAILURE);
    }

    printf("get recv message : %s\n", buffer);
    int shmfd = *(int *)CMSG_DATA(cmsghdr);
    if (shmfd < 3)
    {
        perror("Invalid control message on shmfd");
        exit(EXIT_FAILURE);
    }

    void *shmptr = mmap(NULL, MMAP_DATA_LEN, PROT_READ, MAP_SHARED, shmfd, 0);
    if (shmptr == MAP_FAILED)
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    printf("get shared memory message : %s\n", (char *)shmptr);
    munmap(shmptr, MMAP_DATA_LEN);

    close(shmfd);
    close(sockfd);

    return 0;
}