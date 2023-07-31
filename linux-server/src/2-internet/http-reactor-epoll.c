//
// Created by Administrator on 2023/3/13.
//
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "mimetype.h"
#include "websocket/websocket.h"

#define STATIC_PATH "/assets"

#define PATH_LEN 256

#define CONTENT_TYPE_LEN 40
#define ITEM_LENGTH 1024
#define BUFFER_LENGTH 1024
#define MAX_PORT 1
#define CHECK_RET(expr, fnName, ret) {                                          \
            if (expr)                                                           \
            {                                                                   \
                printf(#fnName" in %s err %s\n", __func__, strerror(errno));    \
                return ret;                                                     \
            }                                                                   \
        }
#define SET_FLAG_NONBLOCK(fd) {             \
            int flag = fcntl(fd, F_GETFL);  \
            flag |= O_NONBLOCK;             \
            fcntl(fd, F_SETFL, flag);       \
        }
#define UNSET_FLAG_NONBLOCK(fd) {           \
            int flag = fcntl(fd, F_GETFL);  \
            flag &= ~O_NONBLOCK;            \
            fcntl(fd, F_SETFL, flag);       \
        }
typedef int (*Callback) (int fd, void *arg);
int recvCallback (int clientfd, void *arg);
int sendCallback (int clientfd, void *arg);
struct HttpRequest
{
    int method;
    char url[PATH_LEN];
    char path[PATH_LEN];
    char host[PATH_LEN];
    char userAgent[PATH_LEN];
    char version[5];
    char **query;
    int queryLen;

    char websocketKey[32];
    int websocketState;
};

struct HttpResponse
{
    off_t length;
    int state;
    char contentType[CONTENT_TYPE_LEN];
    char fullPath[PATH_LEN];
    // off_t wend;
    int isSendHead: 1;
};

struct NtyEvent
{
    int fd;
    int epollEvent;

    void *arg;
    Callback callback;

    char *rBuffer;
    char *wBuffer;
    int rLength;
    int wLength;

    struct HttpRequest *req;
    struct HttpResponse *res;
};

struct EventBlock
{
    struct EventBlock *next;
    struct NtyEvent *events;
};

struct Reactor
{
    int epfd;
    int blockCount;
    struct EventBlock *eventBlock;
    struct EventBlock *endBlock;
};
int eventSet (int epfd, int epollEvent, struct NtyEvent *ev)
{

    struct epoll_event ep_ev = { 0, { 0 }};
    ep_ev.data.ptr = ev;
    ep_ev.events = ev->epollEvent;

    if (epoll_ctl(epfd, epollEvent, ev->fd, &ep_ev) < 0)
    {
        printf(
            "event add failed [fd=%d], events[%d]\n",
            ev->fd,
            ev->epollEvent
        );
        return -1;
    }

    return 0;
}
int eventDel (int epfd, struct NtyEvent *ev)
{
    struct epoll_event ep_ev = { 0, { 0 }};

    ep_ev.data.ptr = ev;
    epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &ep_ev);

    return 0;
}
int reactorResize (struct Reactor *reactor)
{
    if (reactor == NULL)
        return -1;

    struct NtyEvent *ntyEvent = malloc(sizeof(struct NtyEvent) * ITEM_LENGTH);
    CHECK_RET(ntyEvent == NULL, malloc, -3);
    struct EventBlock *block = malloc(sizeof(struct EventBlock));
    if (block == NULL)
    {
        free(ntyEvent);
        CHECK_RET(1, malloc, -3);
    }
    block->events = ntyEvent;
    block->next = reactor->eventBlock ? reactor->eventBlock : block;

    struct EventBlock *eventBlock = reactor->endBlock;
    if (eventBlock != NULL)
        eventBlock->next = block;
    else
        reactor->eventBlock = block;
    reactor->endBlock = block;
    reactor->blockCount++;

    return 0;
}
static struct EventBlock *reactorIndexHelper (struct Reactor *reactor, int fd)
{
    if (reactor == NULL || fd < 3)
        return NULL;

    int index = fd / ITEM_LENGTH;
    while (index >= reactor->blockCount)
        reactorResize(reactor);

    struct EventBlock *block = reactor->eventBlock;
    for (int i = 0; i < index; i++)
        block = block->next;

    return block;
}
struct NtyEvent *reactorIndexFind (struct Reactor *reactor, int fd)
{
    struct EventBlock *block = reactorIndexHelper(reactor, fd);
    return &block->events[fd % ITEM_LENGTH];
}
struct NtyEvent *reactorIndexAdd (struct Reactor *reactor,
    struct NtyEvent *item)
{
    struct EventBlock *block = reactorIndexHelper(reactor, item->fd);
    block->events[item->fd % ITEM_LENGTH] = *item;
    return &block->events[item->fd % ITEM_LENGTH];
}
int reactorIndexDel (struct Reactor *reactor, int fd)
{
    struct EventBlock *block = reactorIndexHelper(reactor, fd);
    struct NtyEvent event = {};
    free(block->events[fd % ITEM_LENGTH].rBuffer);
    free(block->events[fd % ITEM_LENGTH].wBuffer);
    free(block->events[fd % ITEM_LENGTH].req);
    for (int i = 0; i < block->events[fd % ITEM_LENGTH].req->queryLen; ++i)
        free(block->events[fd % ITEM_LENGTH].req->query[i]);
    free(block->events[fd % ITEM_LENGTH].req->query);
    free(block->events[fd % ITEM_LENGTH].res);
    block->events[fd % ITEM_LENGTH].rBuffer = NULL;
    block->events[fd % ITEM_LENGTH].wBuffer = NULL;
    block->events[fd % ITEM_LENGTH].req = NULL;
    block->events[fd % ITEM_LENGTH].res = NULL;
    block->events[fd % ITEM_LENGTH] = event;
    return 0;
}
int reactorInit (struct Reactor *reactor)
{
    if (reactor == NULL)
        return -1;

    memset(reactor, 0, sizeof(struct Reactor));
    reactor->epfd = epoll_create(1);
    if (reactor->epfd < 1)
    {
        close(reactor->epfd);
        CHECK_RET(1, epoll_create, -3);
    }
    return reactorResize(reactor);
}
int reactorAddListener (struct Reactor *reactor,
    int sockfd,
    Callback acceptCb)
{
    if (reactor == NULL || sockfd < 3 || acceptCb == NULL)
        return -1;

    struct NtyEvent item = {};
    item.fd = sockfd;
    item.epollEvent = EPOLLIN;
    item.arg = reactor;
    item.callback = acceptCb;
    // item.rBuffer = calloc(BUFFER_LENGTH, sizeof(char));
    // item.wBuffer = calloc(BUFFER_LENGTH, sizeof(char));

    return eventSet(
        reactor->epfd,
        EPOLL_CTL_ADD,
        reactorIndexAdd(reactor, &item));
}
int reactorRun (struct Reactor *reactor)
{
    if (reactor == NULL || reactor->blockCount < 1 || reactor->epfd < 3
        || reactor->eventBlock == NULL)
        return -1;

    struct epoll_event epollEvent[ITEM_LENGTH] = {};
    int i;
    while (1)
    {
        int nready = epoll_wait(reactor->epfd, epollEvent, ITEM_LENGTH, -1);

        if (nready < 0) continue;

        for (i = 0; i < nready; ++i)
        {
            struct NtyEvent *event = epollEvent[i].data.ptr;

            if (
                ((epollEvent[i].events & EPOLLIN)
                    && (event->epollEvent & EPOLLIN))
                    ||
                        ((epollEvent[i].events & EPOLLOUT)
                            && (event->epollEvent & EPOLLOUT))
                )
                event->callback(event->fd, event->arg);
        }
    }
}
int reactorFree (struct Reactor *reactor)
{
    for (struct EventBlock *block = reactor->eventBlock;
         block != reactor->endBlock; block = block->next)
    {
        for (int i = 0; i < ITEM_LENGTH; ++i)
        {
            for (int j = 0; j < block->events[i].req->queryLen; ++j)
                free(block->events[i].req->query[i]);
            free(block->events[i].req->query);
            free(block->events[i].wBuffer);
            free(block->events[i].rBuffer);
            free(block->events[i].req);
            free(block->events[i].res);
        }
        free(block->events);
    }
    return 0;
}

int createTcpSockfd (int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(sockfd < 0, socket, -3);

    SET_FLAG_NONBLOCK(sockfd);
    int set = 1;
    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &set,
        sizeof(int));

    struct sockaddr_in sockaddrIn = {};
    sockaddrIn.sin_addr.s_addr = INADDR_ANY;
    sockaddrIn.sin_port = htons(port);
    sockaddrIn.sin_family = AF_INET;
    int ret = bind(
        sockfd,
        (const struct sockaddr *)&sockaddrIn,
        sizeof(struct sockaddr));
    CHECK_RET(ret < 0, bind, -3);

    SET_FLAG_NONBLOCK(sockfd);

    ret = listen(sockfd, 10);
    CHECK_RET(ret < 0, bind, -3);
    return sockfd;
}

int readLine (const char *buffer, char *lineBuffer, int pos)
{
    while (buffer[pos] != '\r' && buffer[pos + 1] != '\n')
        *(lineBuffer++) = buffer[pos++];

    *(lineBuffer) = '\0';
    pos += 2;
    if (pos == strlen(buffer))
        return EOF;
    return pos;
}
int checkMethod (const char *methodStr)
{
    if (!strcmp(methodStr, "GET"))
        return GET;
    else if (!strcmp(methodStr, "POST"))
        return POST;
    return -1;
}
int httpRequestQueryParse (struct NtyEvent *event)
{
    char *index = NULL;
    int i;
    for (i = 0; i < strlen(event->req->url); ++i)
    {
        if (*(event->req->url + i) == '?')
        {
            index = event->req->url + i + 1;
            break;
        }
    }
    if (index)
    {
        strncpy(event->req->path, event->req->url, i);
        for (; i < strlen(event->req->url); ++i)
        {
            if (*(event->req->url + i) == '&')
                event->req->queryLen += 2;
        }
        event->req->queryLen += 2;

        event->req->query = calloc(event->req->queryLen, sizeof(char *));
        i = (int)(index - event->req->url);
        int j = 0;
        for (; i < strlen(event->req->url) + 1; ++i)
        {
            if (*(event->req->url + i) == '=' || *(event->req->url + i) == '&'
                || i == strlen(event->req->url))
            {
                event->req->query[j] =
                    calloc((event->req->url + i - index + 1), sizeof(char));
                strncpy(
                    event->req->query[j++],
                    index,
                    event->req->url + i - index
                );
                index = event->req->url + i + 1;
            }
        }
    }
    return 0;
}
int httpFirstHeadParse (struct NtyEvent *event, char *lineBuffer)
{
    char *str = strtok(lineBuffer, " ");
    event->req->method = checkMethod(str);
    str = strtok(NULL, " ");
    strcpy(event->req->url, str);
    // HTTP/1.1
    str = strtok(NULL, " ");
    // HTTP
    str = strtok(str, "/");
    // 1.1
    str = strtok(NULL, "");
    strcpy(event->req->version, str);

    httpRequestQueryParse(event);
    return 0;
}
int httpRequestParse (struct NtyEvent *event)
{
    if (event->req->websocketState == WS_TRANMISSION)
    {
        int ret = websocketRecv(
            &event->req->websocketState,
            event->rBuffer,
            event->wBuffer,
            BUFFER_LENGTH
        );
        event->wLength = strlen(event->wBuffer);
        return ret;
    }
    char *buffer = event->rBuffer;
    char lineBuffer[PATH_LEN] = {};
    int pos = readLine(buffer, lineBuffer, 0);
    httpFirstHeadParse(event, lineBuffer);

    char *str;
    while ((pos = readLine(buffer, lineBuffer, pos)) != EOF)
    {
        printf("%s\n", lineBuffer);
        if (strstr(lineBuffer, "Host"))
        {
            strtok(lineBuffer, " ");
            str = strtok(NULL, "");
            strcpy(event->req->host, str);
        }
        else if (strstr(lineBuffer, "User-Agent"))
        {
            strtok(lineBuffer, " ");
            str = strtok(NULL, "");
            strcpy(event->req->userAgent, str);
        }
        else if (strstr(lineBuffer, "Sec-WebSocket-Key"))
        {
            strtok(lineBuffer, " ");
            str = strtok(NULL, "");
            strcpy(event->req->websocketKey, str);
            websocketRecv(
                &event->req->websocketState,
                event->req->websocketKey,
                event->wBuffer,
                BUFFER_LENGTH
            );
            event->wLength = strlen(event->wBuffer);
        }
    }

    return 0;
}
int getFileContentType (struct NtyEvent *event)
{
    char *ext = strrchr(event->req->path, '.');
    if (ext != NULL)
    {
        ext++;
        getFileMimetype(event->res->contentType, ext);
    }
    else
        strcpy(event->res->contentType, TYPE_HTML);
    return 0;
}
int getHttpResponseHead (struct NtyEvent *event)
{
    char stateStr[40] = {};
    getFileContentType(event);
    getHttpHeadState(event->res->state, stateStr);
    sprintf(
        event->wBuffer,
        "HTTP/1.1 %s\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: %ld\r\n"
        "Content-Type: %s\r\n"
        "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n"
        "Connection: keep-alive\r\n"
        "Keep-Alive: timeout=60\r\n\r\n",
        stateStr,
        event->res->length,
        event->res->contentType
    );
    event->wLength = (int)strlen(event->wBuffer);

    return 0;
}
int httpResponseHeadSend (struct NtyEvent *event)
{
    if (strcmp(event->req->websocketKey, ""))
        return (int)send(event->fd, event->wBuffer, event->wLength, 0);
    char *path = event->req->path;
    memset(event->res->fullPath, 0, PATH_LEN);
    if (!strcmp(path, "/"))
        strcat(event->res->fullPath, "../assets/html/index.html");
    else
    {
        strcat(event->res->fullPath, "..");
        strcat(event->res->fullPath, STATIC_PATH);
        strcat(event->res->fullPath, path);
    }
    // 以STATIC_PATH目录开头
    struct stat fileStat = {};
    stat(event->res->fullPath, &fileStat);
    event->res->length = fileStat.st_size;
    if (S_ISREG(fileStat.st_mode))
        event->res->state = HTTP_OK;
    else
        event->res->state = HTTP_NOTFOUND;

    getHttpResponseHead(event);
    return (int)send(event->fd, event->wBuffer, event->wLength, 0);
}

int httpResponseBodySend (struct NtyEvent *event)
{
    if (event->res->state != HTTP_NOTFOUND)
    {
        if (!resTypeIsFile(event->res->contentType))
        {
            UNSET_FLAG_NONBLOCK(event->fd);
            int filefd = open(event->res->fullPath, O_RDONLY);

            int ret = (int)sendfile(
                event->fd,
                filefd,
                NULL,
                event->res->length
            );
            SET_FLAG_NONBLOCK(event->fd);

            return ret;
        }
    }

    return -3;
}

int acceptCallback (int socket, void *arg)
{
    struct Reactor *reactor = arg;

    struct sockaddr_in sockaddrIn = {};
    socklen_t len = sizeof(struct sockaddr_in);

    int clientfd = accept(socket, (struct sockaddr *)&sockaddrIn, &len);
    CHECK_RET(clientfd == -1, accept, -3);

    SET_FLAG_NONBLOCK(clientfd);
    printf("accept socket %d\n", clientfd);
    struct NtyEvent event = {};
    event.fd = clientfd;
    event.epollEvent = EPOLLIN;
    event.arg = reactor;
    event.callback = recvCallback;
    event.rBuffer = calloc(BUFFER_LENGTH, sizeof(char));
    event.wBuffer = calloc(BUFFER_LENGTH, sizeof(char));
    event.req = calloc(1, sizeof(struct HttpRequest));
    event.res = calloc(1, sizeof(struct HttpResponse));

    return eventSet(
        reactor->epfd,
        EPOLL_CTL_ADD,
        reactorIndexAdd(reactor, &event));
}
int recvCallback (int clientfd, void *arg)
{
    struct Reactor *reactor = arg;

    struct NtyEvent *event = reactorIndexFind(reactor, clientfd);
    CHECK_RET(event == NULL, reactorIndexFind, -3);

    memset(event->rBuffer, 0, BUFFER_LENGTH);
    int len = (int)recv(clientfd, event->rBuffer, BUFFER_LENGTH, 0);
    // printf("recv message success:%d\t%s\n", clientfd, event->rBuffer);
    if (len < 0)
    {
        if (errno == EAGAIN && errno == EWOULDBLOCK)
            return 0;
        perror("recv error");
        close(clientfd);
        eventDel(reactor->epfd, event);
        reactorIndexDel(reactor, clientfd);
        return -1;
    }
    else if (len == 0)
    {
        printf("socket %d close\n", clientfd);
        close(clientfd);
        eventDel(reactor->epfd, event);
        reactorIndexDel(reactor, clientfd);
        return 0;
    }
    else
    {
        // printf("收到信息：\n%s", event->rBuffer);
        httpRequestParse(event);
        event->rLength = len;
        event->epollEvent = EPOLLOUT;
        event->callback = sendCallback;
        return eventSet(reactor->epfd, EPOLL_CTL_MOD, event);
    }
}
int sendCallback (int clientfd, void *arg)
{
    struct Reactor *reactor = arg;

    struct NtyEvent *event = reactorIndexFind(reactor, clientfd);
    CHECK_RET(event == NULL, reactorIndexFind, -3);

    if (!strcmp(event->req->websocketKey, ""))
    {
        memset(event->wBuffer, 0, BUFFER_LENGTH);
        strcpy(event->wBuffer, event->rBuffer);
        event->wLength = event->rLength;
        if (strcmp(event->wBuffer, "") == 0)
            printf("send NULL str clientfd: %d", clientfd);
    }
    // int ret = (int)send(clientfd, event->wBuffer, BUFFER_LENGTH, 0);
    // printf("send message success:%d\t%s\n", clientfd, event->wBuffer);
    int ret = httpResponseHeadSend(event);
    if (!event->res->isSendHead)
    {
        if (ret < 0)
        {
            if (errno == EAGAIN && errno == EWOULDBLOCK)
                return 0;
            perror("http head send error");
            close(clientfd);
            eventDel(reactor->epfd, event);
            reactorIndexDel(reactor, clientfd);
            return -1;
        }
    }
    ret = httpResponseBodySend(event);
    if (ret < 0 && ret != -3)
    {
        if (errno == EAGAIN && errno == EWOULDBLOCK)
            return 0;
        perror("http body send error");
        close(clientfd);
        eventDel(reactor->epfd, event);
        reactorIndexDel(reactor, clientfd);
        return -1;
    }

    event->callback = recvCallback;
    event->epollEvent = EPOLLIN;
    return eventSet(reactor->epfd, EPOLL_CTL_MOD, event);
}
int main ()
{
    setbuf(stdout, NULL);
    struct Reactor *reactor = malloc(sizeof(struct Reactor));
    reactorInit(reactor);

    int listenfd;
    for (int i = 0; i < MAX_PORT; ++i)
    {
        listenfd = createTcpSockfd(3000 + i);
        reactorAddListener(reactor, listenfd, acceptCallback);
    }
    reactorRun(reactor);

    reactorFree(reactor);
    free(reactor);

    return 0;
}