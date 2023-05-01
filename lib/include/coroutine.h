//
// Created by Administrator on 2023/3/30.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_COROUTINE_H_
#define LINUX_SERVER_LIB_INCLUDE_COROUTINE_H_

#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <inttypes.h>

#include "tree.h"
#include "queue.h"

#define _USE_UCONTEXT
#define COROUTINE_HOOK

#define CANCEL_FD_WAIT_UINT64 1
#define COROUTINE_MAX_EVENTS (1024*1024)
// 128K
#define COROUTINE_MAX_STACK_SIZE (128*1024)

#define BIT(x) (1 << (x))
#define CLEAR_BIT(x) ~(1 << (x))

#ifdef _USE_UCONTEXT
#include <ucontext.h>
#endif

typedef void (*procCoroutine) (void *);

// 链表
LIST_HEAD(CoroutineLink, _Coroutine);
// 队列
TAILQ_HEAD(CoroutineQueue, _Coroutine);

RB_HEAD(CoroutineSleep, _Coroutine);
RB_HEAD(CoroutineWait, _Coroutine);

typedef enum
{
    COROUTINE_STATUS_WAIT_READ,
    COROUTINE_STATUS_WAIT_WRITE,
    COROUTINE_STATUS_NEW,
    COROUTINE_STATUS_READY,
    COROUTINE_STATUS_EXITED,
    COROUTINE_STATUS_BUSY,
    COROUTINE_STATUS_SLEEPING,
    COROUTINE_STATUS_EXPIRED,
    COROUTINE_STATUS_FDEOF,
    COROUTINE_STATUS_DETACH,
    COROUTINE_STATUS_CANCELLED,
    COROUTINE_STATUS_PENDING_RUNCOMPUTE,
    COROUTINE_STATUS_RUNCOMPUTE,
    COROUTINE_STATUS_WAIT_IO_READ,
    COROUTINE_STATUS_WAIT_IO_WRITE,
    COROUTINE_STATUS_WAIT_MULTI
} CoroutineStatus;

// 如果不使用UCONTEXT，需要自己实现线程切换
#ifndef _USE_UCONTEXT
typedef struct _CpuCtx {
    void *esp; //
    void *ebp;
    void *eip;
    void *edi;
    void *esi;
    void *ebx;
    void *r1;
    void *r2;
    void *r3;
    void *r4;
    void *r5;
} CpuCtx;
#endif

typedef struct _Schedule
{
    uint64_t birth;

#ifdef _USE_UCONTEXT
    ucontext_t ctx;
#else
    CpuCtx ctx;
#endif

    void *stack;
    size_t stackSize;
    int spawnedCoroutines;
    uint64_t defaultTimeout;
    struct _Coroutine *currThread;
    // 系统分页大小
    // 系统给我们提供真正的内存时，用页为单位提供，一次最少提供一页的真实内存空间
    int pageSize;

    int epfd;
    int eventfd;
    struct epoll_event eventList[COROUTINE_MAX_EVENTS];
    int nEvents;

    int numNewEvents;
    pthread_mutex_t deferMutex;

    CoroutineQueue ready;
    CoroutineQueue defer;

    CoroutineLink busy;

    CoroutineSleep sleeping;
    CoroutineWait waiting;
} Schedule;

typedef struct _Coroutine
{
#ifdef _USE_UCONTEXT
    ucontext_t ctx;
#else
    CpuCtx ctx;
#endif

    procCoroutine func;
    void *arg;

    void *data;
    size_t stackSize;
    size_t lastStackSize;

    CoroutineStatus status;
    // 调度器
    Schedule *schedule;

    uint64_t birth;
    uint64_t id;

#if CANCEL_FD_WAIT_UINT64
    int fd;
    unsigned short events;  //POLL_EVENT
#else
    int64_t fdWait;
#endif

    char funcName[64];
    struct _Coroutine *coJoin;

    void **coExitPtr;
    void *stack;
    void *ebp;
    uint32_t ops;
    uint64_t sleepUSec;

    RB_ENTRY(_Coroutine) sleepNode;
    RB_ENTRY(_Coroutine) waitNode;

    LIST_ENTRY(_Coroutine) busyNode;

    TAILQ_ENTRY(_Coroutine) readyNext;
    TAILQ_ENTRY(_Coroutine) deferNext;
    TAILQ_ENTRY(_Coroutine) condNext;
    TAILQ_ENTRY(_Coroutine) ioNext;
    TAILQ_ENTRY(_Coroutine) computerNext;
} Coroutine;

extern pthread_key_t globalScheduleKey;
static inline Schedule *coroutineGetSchedule ()
{
    return pthread_getspecific(globalScheduleKey);
}

static inline uint64_t coroutineTimeNow (void)
{
    struct timeval t1 = { 0, 0 };
    gettimeofday(&t1, NULL);

    return t1.tv_sec * 1000000 + t1.tv_usec;
}

int coroutineCreate (Coroutine **new_co, procCoroutine func, void *arg);
int coroutineScheduleRun (void);
int coroutineSleep (uint64_t msecs);
void initHook ();
#endif //LINUX_SERVER_LIB_INCLUDE_COROUTINE_H_
