//
// Created by Administrator on 2023/3/30.
//

#include "coroutine.h"

pthread_key_t globalScheduleKey;
pthread_once_t pthreadOnce = PTHREAD_ONCE_INIT;

void coroutineYield (Coroutine *coroutine);
extern void coroutineScheduleSleepDown (Coroutine *, uint64_t);
extern int coroutineScheduleCreate (size_t);
static void coroutineKeyDestroy (void *arg)
{
    free(arg);
}
// 初始化函数，进程只初始化一次
static void coroutineCreateKey (void)
{
    // 线程共享数据，实现同一线程内不同函数之间的数据通信
    // 一个线程一个调度器
    assert(pthread_key_create(&globalScheduleKey, coroutineKeyDestroy) == 0);
    assert(pthread_setspecific(globalScheduleKey, NULL) == 0);
}
#ifdef _USE_UCONTEXT
static void _exec (void *lt)
{
    Coroutine *coroutine = (Coroutine *)lt;
    coroutine->func(coroutine->arg);
    coroutine->status |=
        (BIT(COROUTINE_STATUS_EXITED) | BIT(COROUTINE_STATUS_FDEOF)
            | BIT(COROUTINE_STATUS_DETACH));

    // 执行完成后让出上下文
    coroutineYield(coroutine);
}

static void _save_stack (Coroutine *coroutine)
{
    char *top = coroutine->schedule->stack + coroutine->schedule->stackSize;
    // 在栈中分配一个char，使用栈顶减去char的地址可以得到 当前栈中已分配的大小
    // 因为栈是从高位开始分配
    char dummy = 0;
    assert(top - &dummy <= COROUTINE_MAX_STACK_SIZE);

    // 如果协程的stack大小 小于 已分配大小
    if (coroutine->stackSize < top - &dummy)
    {
        coroutine->stack = realloc(coroutine->stack, top - &dummy);
        assert(coroutine->stack != NULL);
    }
    coroutine->stackSize = top - &dummy;
    // 将当前调度器使用的内存cpy给协程的stack
    memcpy(coroutine->stack, &dummy, top - &dummy);
}

static void _load_stack (Coroutine *coroutine)
{
    memcpy(
        coroutine->schedule->stack + coroutine->schedule->stackSize
            - coroutine->stackSize, coroutine->stack, coroutine->stackSize
    );
}
#else
int _switch (CpuCtx *new_ctx, CpuCtx *cur_ctx);

#ifdef __i386__
__asm__ (
"    .text                                  \n"
"    .p2align 2,,3                          \n"
".globl _switch                             \n"
"_switch:                                   \n"
"__switch:                                  \n"
"movl 8(%esp), %edx      # fs->%edx         \n"
"movl %esp, 0(%edx)      # save esp         \n"
"movl %ebp, 4(%edx)      # save ebp         \n"
"movl (%esp), %eax       # save eip         \n"
"movl %eax, 8(%edx)                         \n"
"movl %ebx, 12(%edx)     # save ebx,esi,edi \n"
"movl %esi, 16(%edx)                        \n"
"movl %edi, 20(%edx)                        \n"
"movl 4(%esp), %edx      # ts->%edx         \n"
"movl 20(%edx), %edi     # restore ebx,esi,edi      \n"
"movl 16(%edx), %esi                                \n"
"movl 12(%edx), %ebx                                \n"
"movl 0(%edx), %esp      # restore esp              \n"
"movl 4(%edx), %ebp      # restore ebp              \n"
"movl 8(%edx), %eax      # restore eip              \n"
"movl %eax, (%esp)                                  \n"
"ret                                                \n"
);
#elif defined(__x86_64__)

__asm__ (
    "    .text                                  \n"
    "       .p2align 4,,15                                   \n"
    ".globl _switch                                          \n"
    ".globl __switch                                         \n"
    "_switch:                                                \n"
    "__switch:                                               \n"
    "       movq %rsp, 0(%rsi)      # save stack_pointer     \n"
    "       movq %rbp, 8(%rsi)      # save frame_pointer     \n"
    "       movq (%rsp), %rax       # save insn_pointer      \n"
    "       movq %rax, 16(%rsi)                              \n"
    "       movq %rbx, 24(%rsi)     # save rbx,r12-r15       \n"
    "       movq %r12, 32(%rsi)                              \n"
    "       movq %r13, 40(%rsi)                              \n"
    "       movq %r14, 48(%rsi)                              \n"
    "       movq %r15, 56(%rsi)                              \n"
    "       movq 56(%rdi), %r15                              \n"
    "       movq 48(%rdi), %r14                              \n"
    "       movq 40(%rdi), %r13     # restore rbx,r12-r15    \n"
    "       movq 32(%rdi), %r12                              \n"
    "       movq 24(%rdi), %rbx                              \n"
    "       movq 8(%rdi), %rbp      # restore frame_pointer  \n"
    "       movq 0(%rdi), %rsp      # restore stack_pointer  \n"
    "       movq 16(%rdi), %rax     # restore insn_pointer   \n"
    "       movq %rax, (%rsp)                                \n"
    "       ret                                              \n"
    );
#endif
static void _exec (void *lt)
{
#if defined(__lvm__) && defined(__x86_64__)
    __asm__("movq 16(%%rbp), %[lt]" : [lt] "=r" (lt));
#endif

    Coroutine *co = (Coroutine *)lt;
    co->func(co->arg);
    co->status |= (BIT(COROUTINE_STATUS_EXITED) | BIT(COROUTINE_STATUS_FDEOF)
        | BIT(COROUTINE_STATUS_DETACH));
#if 1
    coroutineYield(co);
#else
    co->ops = 0;
    _switch(&co->sched->ctx, &co->ctx);
#endif
}

static inline void coroutineMadvise (Coroutine *co)
{

    size_t current_stack = (co->stack + co->stackSize) - co->ctx.esp;
    assert(current_stack <= co->stackSize);

    if (current_stack < co->lastStackSize &&
        co->lastStackSize > co->schedule->pageSize)
    {
        size_t tmp =
            current_stack + (-current_stack & (co->schedule->pageSize - 1));
        assert(madvise(co->stack, co->stackSize - tmp, MADV_DONTNEED) == 0);
    }
    co->lastStackSize = current_stack;
}
#endif

void coroutineFree (Coroutine *coroutine)
{
    if (coroutine == NULL) return;
    coroutine->schedule->spawnedCoroutines--;

    if (coroutine->stack)
    {
        free(coroutine->stack);
        coroutine->stack = NULL;
    }

    free(coroutine);
}

static void coroutineInit (Coroutine *coroutine)
{
#ifdef _USE_UCONTEXT
    getcontext(&coroutine->ctx);
    coroutine->ctx.uc_stack.ss_sp = coroutine->schedule->stack;
    coroutine->ctx.uc_stack.ss_size = coroutine->schedule->stackSize;
    // 设置当前context执行结束后执行的下个context
    coroutine->ctx.uc_link = &coroutine->schedule->ctx;
    // 修改上下文，给上下文设置一个栈空间uc_stack，设置一个后继上下文uc_link
    // 当通过setcontext或swapcontext激活后，执行_exec函数
    makecontext(&coroutine->ctx, (void (*))_exec, 1, coroutine);
#else
    // 汇编实现，保存当前协程的上下文(coroutine->ctx %rsi)
    // 保存到调度器的上下文中(coroutine->schedule->ctx %rdi)
    _switch(&coroutine->schedule->ctx, &coroutine->ctx);
#endif
    // 切换状态为就绪态
    coroutine->status = BIT(COROUTINE_STATUS_READY);
}
int coroutineCreate (Coroutine **newCo, procCoroutine func, void *arg)
{
    assert(pthread_once(&pthreadOnce, coroutineCreateKey) == 0);

    Schedule *schedule = coroutineGetSchedule();

    if (schedule == NULL)
    {
        coroutineScheduleCreate(0);

        schedule = coroutineGetSchedule();
        if (schedule == NULL)
        {
            printf("Failed to create scheduler\n");
            return -1;
        }
    }

    Coroutine *coroutine = calloc(1, sizeof(Coroutine));
    if (coroutine == NULL)
    {
        printf("Failed to allocate memory for new coroutine\n");
        return -2;
    }

#ifdef _USE_UCONTEXT
    coroutine->stack = NULL;
    coroutine->stackSize = 0;
#else
    int ret = posix_memalign(
        &coroutine->stack,
        schedule->pageSize,
        schedule->stackSize
    );
    if (ret)
    {
        printf("Failed to allocate stack for new coroutine\n");
        free(coroutine);
        return -3;
    }
    coroutine->stackSize = schedule->stackSize;
#endif

    coroutine->schedule = schedule;
    // 设置状态为新创建
    coroutine->status = BIT(COROUTINE_STATUS_NEW);
    coroutine->id = schedule->spawnedCoroutines++;
    coroutine->func = func;

#if CANCEL_FD_WAIT_UINT64
    coroutine->fd = -1;
    coroutine->events = 0;
#else
    coroutine->fdWait = -1;
#endif

    coroutine->arg = arg;
    coroutine->birth = coroutineTimeNow();
    *newCo = coroutine;

    TAILQ_INSERT_TAIL(&coroutine->schedule->ready, coroutine, readyNext);

    return 0;
}

int coroutineResume (Coroutine *coroutine)
{
    Schedule *schedule = coroutineGetSchedule();
    schedule->currThread = coroutine;

    // 如果是新创建的协程
    if (coroutine->status & BIT(COROUTINE_STATUS_NEW))
        coroutineInit(coroutine);
#ifdef _USE_UCONTEXT
    else _load_stack(coroutine);

    // 保存当前上下文到调度器中，切换到当前协程的上下文
    // 当前上下文即coroutineScheduleRun()
    swapcontext(&schedule->ctx, &coroutine->ctx);
#else
    _switch(&coroutine->ctx, &coroutine->schedule->ctx);
    coroutineMadvise(coroutine);
#endif

    schedule->currThread = NULL;

    if (coroutine->status & BIT(COROUTINE_STATUS_EXITED))
    {
        if (coroutine->status & BIT(COROUTINE_STATUS_DETACH))
        {
            printf("协程%lu执行完毕", coroutine->id);
            coroutineFree(coroutine);
        }
        return -1;
    }

    return 0;
}

void coroutineYield (Coroutine *coroutine)
{
    coroutine->ops = 0;
#ifdef _USE_UCONTEXT
    if ((coroutine->status & BIT(COROUTINE_STATUS_EXITED)) == 0)
    {
        // 保存调度器中当前上下文（栈内存）
        _save_stack(coroutine);
    }
    // 切换上下文
    // 保存当前上下文到协程中，切换到调度器中的上下文（coroutineScheduleRun()）
    swapcontext(&coroutine->ctx, &coroutine->schedule->ctx);
#else
    _switch(&coroutine->schedule->ctx, &coroutine->ctx);
#endif
}

int coroutineSleep (uint64_t msecs)
{
    Coroutine *coroutine = coroutineGetSchedule()->currThread;

    if (coroutine == NULL)
        return -1;
    if (msecs == 0)
    {
        TAILQ_INSERT_TAIL(&coroutine->schedule->ready, coroutine, readyNext);
        coroutineYield(coroutine);
    }
    else
        coroutineScheduleSleepDown(coroutine, msecs);

    return 0;
}