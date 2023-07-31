//
// Created by 115282 on 2023/7/19.
//

#include <time.h>
#include <hiredis/hiredis.h>
#include <stdlib.h>

#define HOST "127.0.0.1"
#define PORT 6379
#define KEY "user:1000"
#define KEY_INIT_NUM 0
#define KEY_MAX_NUM 300000

int main ()
{
    setbuf(stdout, 0);
    redisContext *ctx = redisConnect(HOST, PORT);
    if (ctx->err)
    {
        fprintf(stdout, "Error in redisAsyncConnect : %s", ctx->errstr);
        exit(EXIT_FAILURE);
    }

    time_t begin = time(NULL);
    redisCommand(ctx, "set "KEY" %d", KEY_INIT_NUM);
    for (int i = 0; i < KEY_MAX_NUM; ++i)
        redisCommand(ctx, "incr "KEY);

    time_t end = time(NULL);
    printf("time used : %lus\n", end - begin);

    return 0;
}