//
// Created by Administrator on 2023/3/30.
//
#include "coroutine.h"

extern int coroutineEpollCreate (void);
extern int coroutineEpollRegisterTrigger (void);
extern int coroutineResume (Coroutine *coroutine);
extern void coroutineFree (Coroutine *coroutine);
extern int coroutineEpollWait (struct timespec *timeval);

static inline int coroutineSleepCmp (Coroutine *c1, Coroutine *c2)
{
    if (c1->sleepUSec < c2->sleepUSec)
        return -1;

    if (c1->sleepUSec == c2->sleepUSec)
        return 0;

    return 1;
}

static inline int coroutineWaitCmp (Coroutine *c1, Coroutine *c2)
{
#if CANCEL_FD_WAIT_UINT64
    if (c1->fd < c2->fd)
        return -1;
    if (c1->fd == c2->fd)
        return 0;
#else
    if (c1->fdWait < c2->fdWait)
        return -1;
    if (c1->fdWait == c2->fdWait)
        return 0;
#endif
    return 1;
}
RB_GENERATE(_CoroutineSleep, _Coroutine, sleepNode, coroutineSleepCmp);
RB_GENERATE(_CoroutineWait, _Coroutine, waitNode, coroutineWaitCmp);

static inline int coroutineScheduleIsDone (Schedule *schedule)
{
    return (RB_EMPTY(&schedule->waiting)
        && RB_EMPTY(&schedule->sleeping)
        && TAILQ_EMPTY(&schedule->ready)
        && LIST_EMPTY(&schedule->busy));
}

static Coroutine *coroutineScheduleExpired (Schedule *schedule)
{
    uint64_t diffTime = coroutineTimeNow() - schedule->birth;
    Coroutine *coroutine = RB_MIN(_CoroutineSleep, &schedule->sleeping);

    if (coroutine == NULL)
        return NULL;

    if (coroutine->sleepUSec <= diffTime)
    {
        RB_REMOVE(_CoroutineSleep, &coroutine->schedule->sleeping, coroutine);
        return coroutine;
    }

    return NULL;
}

static uint64_t coroutineScheduleMinTimeout (Schedule *schedule)
{
    uint64_t diffTime = coroutineTimeNow() - schedule->birth;
    uint64_t timeout = schedule->defaultTimeout;

    // 获取睡眠协程中时间最小的
    Coroutine *coroutine = RB_MIN(_CoroutineSleep, &schedule->sleeping);

    if (!coroutine) return timeout;

    timeout = coroutine->sleepUSec;
    if (timeout > diffTime)
        return timeout - diffTime;

    return 0;
}
static int coroutineScheduleEpoll (Schedule *schedule)
{
    if (schedule == NULL)
        return -1;
    schedule->numNewEvents = 0;

    struct timespec timespec = { 0, 0 };
    // 取出睡眠协程中时间最小的，如果该协程未超时
    // epoll_wait处理一次io
    uint64_t uSecs = coroutineScheduleMinTimeout(schedule);

    if (uSecs && TAILQ_EMPTY(&schedule->ready))
    {
        timespec.tv_sec = uSecs / 1000000u;
        timespec.tv_nsec = uSecs % 1000u * 1000u;
    }
    else
        return 0;

    int nready = 0;
    while (1)
    {
        nready = coroutineEpollWait(&timespec);
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;
            assert(0);
        }

        break;
    }

    schedule->nEvents = 0;
    schedule->numNewEvents = nready;

    return 0;
}

static Coroutine *coroutineScheduleSearchWait (int fd)
{
    Schedule *schedule = coroutineGetSchedule();

    Coroutine tmp = {};
    tmp.fd = fd;

    Coroutine *coroutine = RB_FIND(_CoroutineWait, &schedule->waiting, &tmp);
    coroutine->status = 0;

    return coroutine;
}
void coroutineScheduleFree (Schedule *schedule)
{
    if (schedule->epfd > 0)
        close(schedule->epfd);
    if (schedule->eventfd > 0)
        close(schedule->eventfd);

    free(schedule->stack);
    free(schedule);

    assert(pthread_setspecific(globalScheduleKey, NULL) == 0);
}
int coroutineScheduleCreate (size_t stackSize)
{
    stackSize = stackSize ? stackSize : COROUTINE_MAX_STACK_SIZE;

    Schedule *schedule = calloc(1, sizeof(Schedule));
    if (schedule == NULL)
    {
        printf("Failed to initialize scheduler\n");
        return -1;
    }

    assert(pthread_setspecific(globalScheduleKey, schedule) == 0);

    schedule->epfd = coroutineEpollCreate();
    if (schedule->epfd < 3)
    {
        printf("Failed to initialize epoll\n");
        coroutineScheduleFree(schedule);
        return -2;
    }

    coroutineEpollRegisterTrigger();

    schedule->stackSize = stackSize;
    schedule->pageSize = getpagesize();

#ifdef _USE_UCONTEXT
    // 分配schedule->stackSize的内存，以schedule->pageSize(4096)对齐
    assert(posix_memalign(
        &schedule->stack,
        schedule->pageSize,
        schedule->stackSize
    ) == 0);
#else
    schedule->stack = NULL;
    bzero(&schedule->ctx, sizeof(CpuCtx));
#endif

    schedule->spawnedCoroutines = 0;
    // 3s
    schedule->defaultTimeout = 3000000u;

    RB_INIT(&schedule->sleeping);
    RB_INIT(&schedule->waiting);

    schedule->birth = coroutineTimeNow();

    TAILQ_INIT(&schedule->ready);
    TAILQ_INIT(&schedule->defer);
    LIST_INIT(&schedule->busy);

    return 0;
}

void coroutineScheduleSleepDown (Coroutine *coroutine, uint64_t msecs)
{
    uint64_t uSecs = msecs * 1000u;

    // 先找睡眠协程的红黑树中是否有当前协程
    Coroutine *coroutineTemp =
        RB_FIND(_CoroutineSleep, &coroutine->schedule->sleeping, coroutine);

    // 如果有，删除节点，重新赋值
    if (coroutineTemp != NULL)
        RB_REMOVE(_CoroutineSleep,
                  &coroutine->schedule->sleeping,
                  coroutineTemp);

    coroutine->sleepUSec =
        coroutineTimeNow() - coroutine->schedule->birth + uSecs;

    while (1)
    {
        coroutineTemp = RB_INSERT(_CoroutineSleep,
                                  &coroutine->schedule->sleeping,
                                  coroutine);
        // 如果有时间相等的协程，把当前协程睡眠时间++，然后继续插
        if (coroutineTemp)
        {
            coroutine->sleepUSec++;
            continue;
        }

        coroutine->status |= BIT(COROUTINE_STATUS_SLEEPING);
        break;
    }
}

void coroutineScheduleWait (Coroutine *coroutine,
    int fd,
    unsigned short events,
    uint64_t timeout)
{
    if (coroutine->status & BIT(COROUTINE_STATUS_WAIT_READ)
        || coroutine->status & BIT(COROUTINE_STATUS_WAIT_WRITE))
    {
        // 待读待写的io 不可再次设置wait状态
        printf(
            "Unexpected event. lt id %"PRIu64" fd %"PRId32" already in %"PRId32" state\n",
            coroutine->id, coroutine->fd, coroutine->status
        );
        assert(0);
    }

    if (events & EPOLLIN)
        coroutine->status |= BIT(COROUTINE_STATUS_WAIT_READ);
    else if (events & EPOLLOUT)
        coroutine->status |= BIT(COROUTINE_STATUS_WAIT_WRITE);
    else
    {
        printf("events error: %d", events);
        assert(0);
    }

    coroutine->fd = fd;
    coroutine->events = events;
    Coroutine *coroutineTemp =
        RB_INSERT(_CoroutineWait, &coroutine->schedule->waiting, coroutine);

    // 如果有重复的fd 返回协程指针
    assert(coroutineTemp == NULL);

    if (timeout == 1)
        return;

    // 将当前协程休眠
    coroutineScheduleSleepDown(coroutine, timeout);
}

Coroutine *coroutineScheduleDeWait (int fd)
{
    Coroutine coroutineTemp = {};
    coroutineTemp.fd = fd;

    Schedule *schedule = coroutineGetSchedule();
    assert(schedule != NULL);

    Coroutine *coroutine =
        RB_FIND(_CoroutineWait, &schedule->waiting, &coroutineTemp);
    if (coroutine != NULL)
        RB_REMOVE(_CoroutineWait, &schedule->waiting, coroutine);

    coroutine->status = 0;

    return coroutine;
}

int coroutineScheduleRun (void)
{
    Schedule *schedule = coroutineGetSchedule();

    if (schedule == NULL)
    {
        printf(
            "please init schedule (to run coroutineCreate()), and then run this function\n"
        );
        return -1;
    }

    while (!coroutineScheduleIsDone(schedule))
    {
        Coroutine *coroutine = NULL;
        // 1. expired --> sleep rbtree
        while ((coroutine = coroutineScheduleExpired(schedule)) != NULL)
            // 恢复协程
            coroutineResume(coroutine);

        // 2. ready queue
        Coroutine
            *lastCoroutine = TAILQ_LAST(&schedule->ready, _CoroutineQueue);
        while (!TAILQ_EMPTY(&schedule->ready))
        {
            coroutine = TAILQ_FIRST(&schedule->ready);
            TAILQ_REMOVE(&schedule->ready, coroutine, readyNext);

            // 如果协程状态是EOF，直接退出
            if (coroutine->status & BIT(COROUTINE_STATUS_FDEOF))
            {
                coroutineFree(coroutine);
                break;
            }

            coroutineResume(coroutine);
            if (coroutine == lastCoroutine) break;
        }

        // 3. wait rbtree
        // 此处调用epoll_wait
        coroutineScheduleEpoll(schedule);
        uint32_t eof;
        while (schedule->numNewEvents)
        {
            int index = --schedule->numNewEvents;
            struct epoll_event *event = schedule->eventList + index;

            eof = event->events & EPOLLHUP; // 读写关闭
            if (eof)
                errno = ECONNRESET;

            coroutine = coroutineScheduleSearchWait(event->data.fd);
            if (coroutine != NULL)
            {
                if (eof)
                    coroutine->status |= BIT(COROUTINE_STATUS_FDEOF);

                coroutineResume(coroutine);
            }
            eof = 0;
        }
    }

    coroutineScheduleFree(schedule);

    return 0;
}