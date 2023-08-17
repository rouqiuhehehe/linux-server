//
// Created by 115282 on 2023/6/1.
//

#ifndef HELLO_WORLD_THREAD_POOL_EXECTOR_THREADPOOLEXECTOR_H_
#define HELLO_WORLD_THREAD_POOL_EXECTOR_THREADPOOLEXECTOR_H_

#include "util/global.h"
#include "util/util.h"
#include <thread>
#include <functional>
#include <future>
#include <vector>
#include <list>

class ThreadPoolExecutor : Utils::NonAbleAllCopy, Base
{
protected:
    class Thread : public std::thread
    {
    public:
        template <class ...Arg>
        explicit Thread (Arg &&...arg)
            : std::thread(std::forward <Arg>(arg)...) {}

        ~Thread () noexcept
        {
            if (joinable())
                join();
        }
    };
    class ThreadPoolExecutorPrivate;
    using ThreadPoolExecutorPrivate = ThreadPoolExecutor::ThreadPoolExecutorPrivate;
private:
    DECLARE_PRIVATE_D(ThreadPoolExecutor);
public:
    class Task
    {
        friend ThreadPoolExecutor;
    public:
        Task (
            const std::chrono::milliseconds &expireTime,
            const std::chrono::milliseconds &beginTime
        )
            : expireTime(expireTime), beginTime(beginTime) {}

        __attribute__((unused)) inline bool isRunning () const noexcept { return isRunning_; }
        __attribute__((unused)) inline bool isInvalid () const noexcept { return isInvalid_; }

        inline void setExpireTime (std::chrono::milliseconds time) noexcept
        {
            expireTime = time;
        }
        inline void setFun (std::function <void (const std::shared_ptr <Task> &)> &&fun_) noexcept
        {
            fun = fun_;
        }
    private:
        std::chrono::milliseconds expireTime;
        std::chrono::milliseconds beginTime;
        std::function <void (const std::shared_ptr <Task> &)> fun;
        bool isRunning_ = false;
        bool isInvalid_ = false;
    };

    using TaskPtr = std::shared_ptr <Task>;

    CLASS_IS_VALID(ThreadPoolExecutor, !d->terminate)

    ~ThreadPoolExecutor () noexcept override = default;
    // 单例
    static ThreadPoolExecutor *getInstance (
        uint32_t num = defaultNum,
        uint32_t maxNum = defaultMaxNum,
        const std::chrono::milliseconds &expireTime = std::chrono::milliseconds(0)
    );

    template <class F, class ...Arg>
    inline auto exec (F &&fun, Arg &&...arg) noexcept -> std::future <decltype(fun(arg...))>
    {
        return exec(
            std::chrono::milliseconds(0),
            std::forward <F>(fun),
            std::forward <Arg>(arg)...
        );
    }

    template <class F, class ...Arg>
    inline auto exec (
        const std::chrono::milliseconds &expireTime,
        F &&fun,
        Arg &&...arg
    ) noexcept -> std::future <decltype(fun(arg...))>
    {
        D_PTR(ThreadPoolExecutor);

        auto f = std::make_shared <std::packaged_task <decltype(fun(arg...)) ()>>(
            std::packaged_task <decltype(fun(arg...)) ()>(
                std::bind(
                    std::forward <F>(fun),
                    std::forward <Arg>(arg)...
                ))
        );

        auto beginTaskTime = getTimeNow();

        TaskPtr task = std::make_shared <Task>(expireTime, beginTaskTime);
        task->setFun(
            [this, f] (const TaskPtr &task) {
                auto runTime = getTimeNow();

                task->isRunning_ = true;
                // 执行任务超时
                // 线程默认休眠10毫秒，所以超时时间+10ms
                if (runTime - task->beginTime
                    > (task->expireTime + std::chrono::milliseconds(10)))
                {
                    // 如果返回false不执行任务
                    if (!expireTimeRunWork(task))
                        return;
                }
                if (beforeRunWork(task))
                {
                    (*f)();
                    afterRunWork(task);
                }
            }
        );
        d->tasks.push_back(task);

        // 通知所有是防止任务积压
        d->cond.notify_all();
        return f->get_future();
    }

    void stop ();
    bool isTerminate () const noexcept;
    uint32_t num () const noexcept;
    uint32_t maxNum () const noexcept;
    size_t taskSize () const noexcept;
    size_t threadSize () const noexcept;

protected:
    // 线程数
    // 最大线程数 （=num 即为不扩容）
    // 等待多长时间，开辟新线程处理（回收线程同）
    ThreadPoolExecutor (
        uint32_t num,
        uint32_t maxNum,
        const std::chrono::milliseconds &timeout
    );
    explicit ThreadPoolExecutor (ThreadPoolExecutorPrivate *);

    virtual bool beforeRunWork (const TaskPtr &task);
    virtual void afterRunWork (const TaskPtr &task);
    virtual bool expireTimeRunWork (const TaskPtr &task);

    static uint32_t defaultMaxNum;
    static uint32_t defaultNum;

private:
    bool createThreads ();
    void worker ();

    void createTimerObserverThread ();
    void timerObserverWorker ();
    void extendWorker ();
    void extendWorkerDestroy ();

    static std::chrono::milliseconds getTimeNow ()
    {
        return std::chrono::duration_cast <std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }
protected:
    class ThreadPoolExecutorPrivate : BasePrivate
    {
        typedef std::vector <Thread *> ThreadVector;
        typedef std::list <ThreadPoolExecutor::TaskPtr> TaskList;
        typedef std::unique_lock <std::mutex> UniqueLock;

        DECLARE_PUBLIC_Q(ThreadPoolExecutor); // NOLINT
    public:
        ThreadPoolExecutorPrivate (
            uint32_t num,
            uint32_t maxNum,
            const std::chrono::milliseconds &expireTime,
            ThreadPoolExecutor *parent
        )
            : num(num), maxNum(maxNum), expireTime(expireTime), BasePrivate(parent) {}

        ~ThreadPoolExecutorPrivate () noexcept override
        {
            terminate = true;
            cond.notify_all();
            for (const auto &v : threads)
                delete v;

            for (const auto &v : extendThread.threads)
                delete v;
        }

    private:
        const uint32_t num;
        const uint32_t maxNum;
        std::chrono::milliseconds expireTime;
        ThreadVector threads;
        TaskList tasks {};
        std::unique_ptr <Thread> timerObserver;
        std::mutex mutex;
        std::condition_variable cond;
        std::atomic_int atomic { 0 };
        std::atomic_bool terminate { false };

        struct
        {
            ThreadVector threads;
            std::atomic <int> waitNum { 0 };
            std::condition_variable cond;
        } extendThread;
    };
};

#endif //HELLO_WORLD_THREAD_POOL_EXECTOR_THREADPOOLEXECTOR_H_
