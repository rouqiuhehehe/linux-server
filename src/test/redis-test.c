//
// Created by 115282 on 2023/7/13.
//

#include <hiredis/hiredis.h>
#include <string.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#define HOST "127.0.0.1"
#define PORT 6379

#define KEY "user:1000"

#if 0
#include <stdlib.h>
int redisRedKey (redisContext *redis)
{
    redisReply *res = redisCommand(redis, "get " KEY);
    if (!res)
    {
        fprintf(stderr, "redisCommand error");
        redisFree(redis);
        exit(EXIT_FAILURE);
    }

    if (res->type == REDIS_REPLY_NIL)
        printf("key " KEY " not exist\n");
    else if (res->type == REDIS_REPLY_STRING)
        printf("key " KEY " %s\n", res->str);
    else
        printf("type error\n");

    freeReplyObject(res);
}
int redisSetKey (redisContext *redis, const char *value)
{
    redisReply *res = redisCommand(redis, "set " KEY " %s", value);
    if (!res)
    {
        fprintf(stderr, "redisCommand error");
        redisFree(redis);
        exit(EXIT_FAILURE);
    }

    if (res->type == REDIS_REPLY_STATUS && strcasecmp(res->str, "OK") == 0)
        printf("key " KEY " set success --------> %s\n", value);
    else
        printf("key " KEY " set fail\n");

    freeReplyObject(res);
}
int main ()
{
    redisContext *redis = redisConnect(HOST, PORT);
    if (!redis || redis->err)
    {
        fprintf(stderr, "%s", redis->errstr);
        exit(EXIT_FAILURE);
    }

    redisRedKey(redis);
    redisSetKey(redis, "999999999");
    redisRedKey(redis);

    redisFree(redis);
}
#elif 1

#include <unistd.h>
#include <signal.h>

#define KEY_INIT_NUM 0
#define KEY_MAX_NUM 10000

void connectCallback (const struct redisAsyncContext *redis, int status)
{
    if (status != REDIS_OK)
    {
        fprintf(stderr, "connect error : %s\n", redis->errstr);
        return;
    }

    printf("connect ...\n");
}
void commandCallback (struct redisAsyncContext *redis, void *res, void *privdata)
{
    if (!res) return;
    redisReply *reply = (redisReply *)res;

    if (reply->type == REDIS_REPLY_INTEGER)
    {
        printf("incr "KEY" : %lld", reply->integer);
        if (reply->integer == KEY_MAX_NUM)
        {
            struct event_base *eventBase = (struct event_base *)privdata;
            redisAsyncDisconnect(redis);
        }
    }

    if (reply->type == REDIS_REPLY_STATUS)
    {
        if (strcasecmp(reply->str, "OK") == 0)
            printf("set "KEY" success : %d", KEY_INIT_NUM);
        else
            printf("set "KEY" fail");
    }
}
int main ()
{
    setbuf(stdout, 0);
    struct event_base *eventBase = event_base_new();

    redisAsyncContext *redis = redisAsyncConnect(HOST, PORT);

    redisLibeventAttach(redis, eventBase);

    redisAsyncSetConnectCallback(redis, connectCallback);
    redisAsyncSetDisconnectCallback(redis, connectCallback);

    redisAsyncCommand(redis, commandCallback, NULL, "set "KEY" %d", KEY_INIT_NUM);
    for (int i = 0; i < KEY_MAX_NUM; ++i)
        redisAsyncCommand(redis, commandCallback, eventBase, "incr "KEY);

    event_base_dispatch(eventBase);
    return 0;
}
#endif