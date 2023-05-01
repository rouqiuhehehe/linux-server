//
// Created by Administrator on 2023/2/28.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "include/async.h"
#include <fcntl.h>

#define DNS_HOST "114.114.114.114"
#define DNS_PORT 53
#define REQUEST_SIZE 1024
#define DNS_CNAME_T 0x05
#define DNS_HOST_T 0x01
#define INSERT_LIST(item, list) {    \
            (item)->next = list;     \
            (list) = item;           \
}
#define DELETE_LIST(item, list) {                                        \
            if ((item) == (list)) (list) = (item)->next;                 \
            (item)->next = NULL;                                         \
}
#define SET_FLAG_NONBLOCK(fd) {             \
            int flag = fcntl(fd, F_GETFL);  \
            flag |= O_NONBLOCK;             \
            fcntl(fd, F_SETFL, flag);       \
        }
struct DnsHeader
{
    unsigned short transactionId;
    unsigned short flags;
    // 查询问题区域节的数量
    unsigned short questions;
    // 回答区域的数量
    unsigned short answer;
    // 授权区域的数量
    unsigned short authority;
    // 附加区域的数量
    unsigned short additional;
};

struct DnsBody
{
    int length;
    unsigned short qType;
    unsigned short qClass;
    char *qName;
};

struct DnsAnswerBody
{
    char *name;
    unsigned short qType;
    unsigned short qClass;
    unsigned short ttl;
    unsigned short dataLen;
    char *data;

    struct DnsAnswerBody *next;
};

struct DnsAnswer
{
    struct DnsHeader *header;
    struct DnsAnswerBody *body;
};
static int dnsResponseParse (const char *response, struct DnsAnswer *dnsAnswer);
int dnsCreateHeader (struct DnsHeader *dnsHeader)
{
    if (dnsHeader == NULL)
        return -1;

    memset(dnsHeader, 0, sizeof(struct DnsHeader));

    srandom(time(NULL));
    dnsHeader->transactionId = htons(random());
    // 表示二进制0000000100000000
    // 第一位0表示查询，后0000表示普通查询，后1表示期望递归
    dnsHeader->flags |= htons(0x0100);
    dnsHeader->questions = htons(1);

    return 0;
}

int dnsCreateBody (struct DnsBody *dnsBody, const char *hostname)
{
    if (dnsBody == NULL)
        return -1;
    memset(dnsBody, 0, sizeof(struct DnsBody));

    size_t len = strlen(hostname) + 2;
    dnsBody->qName = malloc(len);
    if (dnsBody->qName == NULL)
        return -2;

    dnsBody->length = (int)len;
    dnsBody->qType = htons(1);
    dnsBody->qClass = htons(1);

    const char delim[2] = ".\0";
    // 开辟一段新空间复制字符串
    char *hostnameTemp = strdup(hostname);
    char *token = strtok(hostnameTemp, delim);

    char *qNamePtr = dnsBody->qName;
    while (token != NULL)
    {
        size_t length = strlen(token);

        *qNamePtr++ = (char)length;
        strcpy(qNamePtr, token);
        // 此时qNamePtr指向末尾的\0
        qNamePtr += length;
        // 参数传NULL，继续上一次结果执行
        token = strtok(NULL, delim);
    }

    free(hostnameTemp);
    return 0;
}

int dnsMergeRequest (
    struct DnsHeader *dnsHeader,
    struct DnsBody *dnsBody,
    char *request,
    size_t len
)
{
    if (dnsHeader == NULL || dnsBody == NULL || request == NULL || len < 1)
        return -1;

    int offset = sizeof(struct DnsHeader);
    memset(request, 0, len);
    // merge 头部
    memcpy(request, dnsHeader, offset);
    // merge body
    memcpy(request + offset, dnsBody->qName, dnsBody->length);
    offset += dnsBody->length;
    memcpy(request + offset, &dnsBody->qType, sizeof(dnsBody->qType));
    offset += sizeof(dnsBody->qType);
    memcpy(request + offset, &dnsBody->qClass, sizeof(dnsBody->qClass));
    offset += sizeof(dnsBody->qClass);

    return offset;
}
int dnsConnection (const char *hostname, struct AsyncInfo *asyncInfo)
{
    if (hostname == NULL)
        return -1;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -2;

    SET_FLAG_NONBLOCK(fd);

    struct sockaddr_in sockaddr = {};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(DNS_PORT);
    sockaddr.sin_addr.s_addr = htonl(inet_addr(DNS_HOST));

    // 此处其实是不需要connect的，但是由于udp第一次连接很容易丢包
    // 所以此处先连接一次
    int ret =
        connect(fd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr));

    struct DnsHeader dnsHeader = {};
    struct DnsBody dnsBody = {};
    dnsCreateHeader(&dnsHeader);
    dnsCreateBody(&dnsBody, hostname);

    char request[REQUEST_SIZE];
    int size = dnsMergeRequest(&dnsHeader, &dnsBody, request, REQUEST_SIZE);

    ret = sendto(
        fd,
        request,
        size,
        0,
        (const struct sockaddr *)&sockaddr,
        sizeof(struct sockaddr));

    if (ret == -1)
        return -3;

    struct epoll_event event = {};
    event.events = EPOLLIN;
    event.data.fd = fd;

    epoll_ctl(asyncInfo->epfd, EPOLL_CTL_ADD, fd, &event);

    free(dnsBody.qName);

    // char *response[REQUEST_SIZE];
    // struct sockaddr_in responseSockaddr = {};
    // socklen_t len = sizeof(struct sockaddr_in);
    //
    // ret = recvfrom(
    //     fd,
    //     response,
    //     REQUEST_SIZE,
    //     0,
    //     (struct sockaddr *)&responseSockaddr,
    //     &len
    // );
    // if (ret == -1)
    //     perror("recvfrom error");
    //
    // struct DnsAnswer *dnsAnswer = (struct DnsAnswer *)asyncInfo->arg;
    // if (dnsResponseParse((const char *)response, dnsAnswer) != 0)
    //     perror("dnsResponseParse error");
    //
    // printf("%s\n", dnsAnswer->body->data);
    return ret;
}
static int is_pointer (int in)
{
    return ((in & 0xC0) == 0xC0);
}

static void dnsParseName (
    unsigned char *chunk,
    unsigned char *ptr,
    char *out,
    int *len
)
{

    int flag = 0, n = 0, alen = 0;
    char *pos = out + (*len);

    while (1)
    {

        flag = (int)ptr[0];
        if (flag == 0) break;

        if (is_pointer(flag))
        {

            n = (int)ptr[1];
            ptr = chunk + n;
            dnsParseName(chunk, ptr, out, len);
            break;

        }
        else
        {

            ptr++;
            memcpy(pos, ptr, flag);
            pos += flag;
            ptr += flag;

            *len += flag;
            if ((int)ptr[0] != 0)
            {
                memcpy(pos, ".", 1);
                pos += 1;
                (*len) += 1;
            }
        }

    }

}

static int dnsResponseParse (const char *response, struct DnsAnswer *dnsAnswer)
{
    if (response == NULL)
        return -1;
    struct DnsHeader *dnsHeader = malloc(sizeof(struct DnsHeader));
    const unsigned char *ptr = response;
    // 头部分
    dnsHeader->transactionId = ntohs(*(unsigned short *)ptr);
    ptr += 2;
    dnsHeader->flags = ntohs(*(unsigned short *)ptr);
    ptr += 2;
    dnsHeader->questions = ntohs(*(unsigned short *)ptr);
    ptr += 2;
    dnsHeader->answer = ntohs(*(unsigned short *)ptr);
    ptr += 2;
    dnsHeader->authority = ntohs(*(unsigned short *)ptr);
    ptr += 2;
    dnsHeader->additional = ntohs(*(unsigned short *)ptr);
    ptr += 2;

    int rcode = (dnsHeader->flags >> 4 << 4) ^ dnsHeader->flags;
    if (rcode != 0)
    {
        if (rcode == 2)
            perror("name error");
        else if (rcode == 3)
            perror("server error");
        return -2;
    }
    // body部分
    int i;
    // 首先是qName，格式为3www5baidu3com0
    for (i = 0; i < dnsHeader->questions; ++i)
    {
        while (1)
        {
            // 3www
            // 5baidu
            // 3com
            int flag = (int)ptr[0];
            ptr += flag + 1;
            if (flag == 0) break;
        }

        // qType 2字节
        // qClass 2字节
        ptr += 4;
    }

    memset(dnsAnswer, 0, sizeof(struct DnsAnswer));
    dnsAnswer->header = dnsHeader;
    int len = 0;
    for (i = 0; i < dnsHeader->answer; i++)
    {
        struct DnsAnswerBody
            *dnsAnswerBody = malloc(sizeof(struct DnsAnswerBody));
        dnsAnswerBody->name = malloc(128);
        len = 0;
        dnsParseName(response, ptr, dnsAnswerBody->name, &len);
        ptr += 2;

        dnsAnswerBody->qType = ntohs(*(unsigned short *)ptr);
        ptr += 2;
        dnsAnswerBody->qClass = ntohs(*(unsigned short *)ptr);
        ptr += 2;
        dnsAnswerBody->ttl = ntohs(*(unsigned short *)ptr);
        ptr += 4;
        dnsAnswerBody->dataLen = ntohs(*(unsigned short *)ptr);
        ptr += 2;

        switch (dnsAnswerBody->qType)
        {
            case DNS_CNAME_T:
                len = 0;
                dnsAnswerBody->data =
                    malloc(255);
                dnsParseName(response, ptr, dnsAnswerBody->data, &len);
                break;
            case DNS_HOST_T:
                // 如果是ip，dataLen一定是4
                if (dnsAnswerBody->dataLen == 4)
                {
                    dnsAnswerBody->data =
                        malloc(sizeof(dnsAnswerBody->dataLen * 4));
                    memcpy(dnsAnswerBody->data, ptr, dnsAnswerBody->dataLen);
                    inet_ntop(
                        AF_INET, dnsAnswerBody->data, dnsAnswerBody->data,
                        sizeof(struct sockaddr));
                }
        }
        ptr += dnsAnswerBody->dataLen;
        INSERT_LIST(dnsAnswerBody, dnsAnswer->body);
    }

    return 0;
}
void freeDnsAnswer (struct DnsAnswer *dnsAnswer)
{
    struct DnsAnswerBody *p = NULL;
    for (struct DnsAnswerBody *i = dnsAnswer->body; i != NULL; i = i->next)
    {
        free(i->name);
        free(i->data);
        free(p);
        p = i;
    }
    free(dnsAnswer->header);
    free(dnsAnswer);
}

void dnsParseCallback (struct AsyncInfo *info, struct epoll_event *event)
{
    int ret;
    char *response[REQUEST_SIZE];
    struct sockaddr_in responseSockaddr = {};
    socklen_t len = sizeof(struct sockaddr_in);

    ret = recvfrom(
        event->data.fd,
        response,
        REQUEST_SIZE,
        0,
        (struct sockaddr *)&responseSockaddr,
        &len
    );
    if (ret == -1)
        perror("recvfrom error");

    struct DnsAnswer *dnsAnswer = (struct DnsAnswer *)info->arg;
    if (dnsResponseParse((const char *)response, dnsAnswer) != 0)
        perror("dnsResponseParse error");

    printf("%s\n", dnsAnswer->body->data);
}
int main ()
{
    setbuf(stdout, NULL);
    char *domain[] = {
        "www.ntytcp.com",
        "bojing.wang",
        "www.baidu.com",
        "tieba.baidu.com",
        "news.baidu.com",
        "zhidao.baidu.com",
        "music.baidu.com",
        "image.baidu.com",
        "v.baidu.com",
        "map.baidu.com",
        "baijiahao.baidu.com",
        "xueshu.baidu.com",
        "cloud.baidu.com",
        "www.163.com",
        "open.163.com",
        "auto.163.com",
        "gov.163.com",
        "money.163.com",
        "sports.163.com",
        "tech.163.com",
        "edu.163.com",
        "www.taobao.com",
        "q.taobao.com",
        "sf.taobao.com",
        "yun.taobao.com",
        "baoxian.taobao.com",
        "www.tmall.com",
        "suning.tmall.com",
        "www.tencent.com",
        "www.qq.com",
        "www.aliyun.com",
        "www.ctrip.com",
        "hotels.ctrip.com",
        "hotels.ctrip.com",
        "vacations.ctrip.com",
        "flights.ctrip.com",
        "trains.ctrip.com",
        "bus.ctrip.com",
        "car.ctrip.com",
        "piao.ctrip.com",
        "tuan.ctrip.com",
        "you.ctrip.com",
        "g.ctrip.com",
        "lipin.ctrip.com",
        "ct.ctrip.com"
    };
    struct DnsAnswer *dnsAnswer = malloc(sizeof(struct DnsAnswer));
    struct AsyncInfo *info = malloc(sizeof(struct AsyncInfo));

    asyncIOInit(info, dnsParseCallback, dnsAnswer);

    for (int i = 0; i < sizeof(domain) / sizeof(domain[0]); ++i)
    {
        dnsConnection(domain[i], info);
    }
    getchar();

    freeDnsAnswer(dnsAnswer);
    asyncIODestroy(&info);
    return 0;
}