//
// Created by 115282 on 2023/5/29.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "shm-define.h"

void exitUnlinkShareMemory ()
{
    shm_unlink(FILE_NAME);
}
int setShareMemory ()
{
    int shmfd = shm_open(FILE_NAME, O_RDWR | O_CREAT, 0666);
    if (shmfd == -1)
    {
        perror("shmfd error");
        exit(EXIT_FAILURE);
    }
    printf("shmfd : %d\n", shmfd);

    // 设置共享内存大小
    ftruncate(shmfd, MMAP_DATA_LEN);
    // MAP_SHARED 建立共享，用于进程间通信，如果没有这个标志，则别的进程即使能打开文件，也看不到数据。
    void *shmptr = mmap(NULL, MMAP_DATA_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shmptr == MAP_FAILED)
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    memcpy(shmptr, SHM_MESSAGE, SHM_MESSAGE_LEN);

    munmap(shmptr, MMAP_DATA_LEN);

    // atexit(exitUnlinkShareMemory);

    return shmfd;
}

void setSendmsg (int sockfd, int shmfd, struct sockaddr_un *un, size_t uLen)
{
    struct msghdr msgHdr = {};
    // 套接口地址成员 msg_name 与 msg_namelen
    msgHdr.msg_name = un;
    msgHdr.msg_namelen = uLen;

    struct iovec iov = {};
    iov.iov_base = SEND_MESSAGE;
    iov.iov_len = SEND_MESSAGE_LEN;
    // 设置io vector
    msgHdr.msg_iov = &iov;
    msgHdr.msg_iovlen = 1;

    size_t len = CMSG_LEN(sizeof(int));
    char msgBuffer[CMSG_SPACE(sizeof(int))] = {};
    msgHdr.msg_control = msgBuffer;
    msgHdr.msg_controllen = len;

    // 返回指向附属数据缓冲区内的第一个附属对象的 struct cmsghdr 指针。如果不存在附属数据对象则返回的指针值为 NULL 。
    struct cmsghdr *cmsghdr = CMSG_FIRSTHDR(&msgHdr);
    cmsghdr->cmsg_level = SOL_SOCKET;
    // 附属数据对象是文件描述符
    cmsghdr->cmsg_type = SCM_RIGHTS;
    cmsghdr->cmsg_len = len;
    *(int *)CMSG_DATA(cmsghdr) = shmfd;
    printf("shmfd : %d\n", shmfd);

    if (sendmsg(sockfd, &msgHdr, 0) == -1)
    {
        perror("sendmsg error");
        exit(EXIT_FAILURE);
    }
}

int main ()
{
    int shmfd = setShareMemory();
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 3)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un un = {};
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, SOCK_PATH, sizeof(un.sun_path) - 1);

    setSendmsg(sockfd, shmfd, &un, sizeof(un));
    close(shmfd);
    close(sockfd);
    return 0;
}