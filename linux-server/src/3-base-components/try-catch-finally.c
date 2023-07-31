//
// Created by Yoshiki on 2023/6/18.
//
#include <pthread.h>
#include <setjmp.h>
#include "printf-color.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

pthread_key_t gStack;
pthread_once_t pthreadOnce = PTHREAD_ONCE_INIT;

#define ERROR_MSG_SIZE 1024
#define CALLER_SIZE 10
#define CALLER_INFO(s) \
            void *buffer[CALLER_SIZE]; \
            int size = backtrace(buffer, CALLER_SIZE); \
            char **strings = backtrace_symbols(buffer, size);   \
            int offset = 0;           \
            for (int i = 0; i < size; ++i)  \
            {      \
                sprintf(s + offset, "\t\t\tat %s\n", strings[i]);       \
                offset = strlen(s);       \
            }          \
            *(s + offset) = '\0';  \
            free(strings);

// ExceptionFrame设计成反向链表，解决try catch嵌套问题
// 每一次try都添加一个新的ExceptionFrame，并把prev指向外层的ExceptionFrame
#define Try  \
    volatile int flag; \
    ExceptionFrame frame; \
    frame.prev = pthread_getspecific(gStack); \
    pthread_setspecific(gStack, &frame);      \
    flag = setjmp(frame.env);                 \
    if (1)   {

#define Catch(e) }

typedef struct
{
    const char *message;
} Exception;

typedef struct __ExceptionFrame
{
    jmp_buf env;

    int line;
    const char *funName;
    const char *file;

    Exception *exception;
    struct __ExceptionFrame *prev;
} ExceptionFrame;

static void init ()
{
    pthread_key_create(&gStack, NULL);
}

void exceptionThrow (Exception *exception)
{

    ExceptionFrame *frame = pthread_getspecific(gStack);
    if (frame)
    {

    }
    else
    {
        char errorMsg[ERROR_MSG_SIZE];
        sprintf(errorMsg, "%s\n%c", exception->message, '\0');
        CALLER_INFO(errorMsg + strlen(errorMsg));
        PRINTF_ERROR("%s", errorMsg);
    }
}
void ExceptionInit ()
{
    pthread_once(&pthreadOnce, init);
}

int main ()
{
    // Try
    //     {
    //
    //     }
    // Catch(e)
    // {
    //
    // }
    Exception exception = {
        "ccccccccccc"
    };

    exceptionThrow(&exception);

    return 0;
}