//
// Created by 115282 on 2023/7/19.
//

#include <time.h>
#include "redis-async.h"

#define HOST "127.0.0.1"
#define PORT 6379
#define KEY "user:1000"
#define KEY_INIT_NUM 0
#define KEY_MAX_NUM 300000

static Reactor *reactor;

void commandCallback (
    struct redisAsyncContext *redis,
    void *res,
    __attribute__((unused)) void *privdata
)
{
    if (!res) return;
    redisReply *reply = (redisReply *)res;

    switch (reply->type)
    {
        case REDIS_REPLY_STATUS:
            if (strcasecmp(reply->str, "OK") == 0)
                printf("redis set "KEY" successful\n");
            else
                fprintf(stdout, "redis set "KEY" fail");
            break;
        case REDIS_REPLY_ERROR:
            fprintf(stdout, "redis error : %s", reply->str);
            break;
        case REDIS_REPLY_INTEGER:
            if (reply->integer == KEY_MAX_NUM)
                redisAsyncDisconnect(redis);
            break;
    }
}

void connectCallback (const redisAsyncContext *ctx, int status)
{
    if (status != REDIS_OK)
    {
        fprintf(stdout, "Error in connectCallback : %s", ctx->errstr);
        reactorStop(reactor);
        return;
    }

    printf("Connected...\n");
}
void disconnectCallback (const redisAsyncContext *ctx, int status)
{
    if (status != REDIS_OK)
        fprintf(stdout, "Error in disconnectCallback : %s", ctx->errstr);

    printf("disconnected...\n");
    reactorStop(reactor);
}
int main ()
{
    setbuf(stdout, 0);
    redisAsyncContext *ctx = redisAsyncConnect(HOST, PORT);
    if (ctx->err)
    {
        fprintf(stdout, "Error in redisAsyncConnect : %s", ctx->errstr);
        exit(EXIT_FAILURE);
    }
    reactor = reactorCreate();
    redisAttach(reactor, ctx);

    redisAsyncSetConnectCallback(ctx, connectCallback);
    redisAsyncSetDisconnectCallback(ctx, disconnectCallback);

    time_t begin = time(NULL);
    redisAsyncCommand(ctx, commandCallback, NULL, "set "KEY" %d", KEY_INIT_NUM);
    for (int i = 0; i < KEY_MAX_NUM; ++i)
        redisAsyncCommand(ctx, commandCallback, NULL, "incr "KEY);

    reactorRun(reactor);

    time_t end = time(NULL);
    printf("time used : %lus\n", end - begin);

    reactorDestroy(reactor);
}