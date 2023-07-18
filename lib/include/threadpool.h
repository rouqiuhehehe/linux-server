//
// Created by Administrator on 2023/4/27.
//

#ifndef LINUX_SERVER_LIB_THREAD_MYTHREAD_H_
#define LINUX_SERVER_LIB_THREAD_MYTHREAD_H_

#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <atomic>
#include <future>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

class ThreadPool
{
protected:
    class TaskFun
    {
        friend class ThreadPool;
    public:
        explicit TaskFun (uint64_t expireTime = 0)
            : expireTime_(expireTime) {};

        bool isValid () const { return func_ == nullptr; };
        uint64_t expireTime () const { return expireTime_; };
    private:
        std::function <void ()> func_ = nullptr;
        uint64_t expireTime_;
    };

    typedef std::shared_ptr <TaskFun> TaskFunPtr;
    typedef std::unique_lock <std::mutex> UniqueLock;
public:
    explicit ThreadPool (int num = 10);
    virtual ~ThreadPool ();

    inline size_t getThreadNum ();
    inline size_t getJobNum ();

    void stop ();
    bool start ();

    template <class F, class ...Arg>
    auto exec (uint64_t timeoutMs, F &&f, Arg &&... arg) -> std::future <decltype(f(arg...))>;

    template <class F, class ...Arg>
    auto exec (F &&f, Arg &&... arg) -> std::future <decltype(f(arg...))>;

    /**
    * @brief 等待当前任务队列中, 所有工作全部结束(队列无任务).
    *
    * @param millsecond 等待的时间(ms), -1:永远等待
    * @return           true, 所有工作都处理完毕
    *                   false,超时退出
    */
    void waitForAllDone (int = -1);

protected:
    /**
    * @brief 获取任务
    *
    * @return TaskFuncPtr
    */
    void run ();

    TaskFunPtr get ();

    inline bool isTerminate () const;

    static inline void getNow (timeval *tv);
    static inline uint64_t getNowMs ();

    virtual bool beforeRunTask (const TaskFunPtr &) { return true; };
    virtual void afterRunTask (const TaskFunPtr &) {};
    virtual void errorHandler () {};
    virtual void expireTaskHandler (const TaskFunPtr &) {};

protected:
    std::queue <TaskFunPtr> tasks_;
    std::vector <std::thread *> threads_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t threadNum_;
    bool terminate_ = false;
    std::atomic <int> atomic_ { 0 };
};

size_t ThreadPool::getThreadNum ()
{
    UniqueLock lock(mutex_);

    return threads_.size();
}
size_t ThreadPool::getJobNum ()
{
    UniqueLock lock(mutex_);
    return tasks_.size();
}
bool ThreadPool::isTerminate () const
{
    return terminate_;
}
template <class F, class... Arg>
auto ThreadPool::exec (F &&f, Arg &&... arg) -> std::future <decltype(f(arg...))>
{
    return execf(0, f, arg...);
}
template <class F, class... Arg>
auto ThreadPool::exec (uint64_t timeoutMs, F &&f, Arg &&... arg) -> std::future <decltype(f(arg...))>
{
    uint64_t expireTime = timeoutMs == 0 ? 0 : (getNowMs() + timeoutMs);

    using RetType = decltype(f(arg...));
    auto task =
        std::make_shared <std::packaged_task <RetType ()>>(std::bind(std::forward <F>(f), std::forward <Arg>(arg)...));

    TaskFunPtr ptr = std::make_shared <TaskFun>(expireTime);
    ptr->func_ = [this, ptr, task] () {
        if (!beforeRunTask(ptr))
            return;
        (*task)();
        afterRunTask(ptr);
    };

    UniqueLock lock(mutex_);
    tasks_.push(ptr);
    condition_.notify_one();

    return task->get_future();
}

void ThreadPool::getNow (timeval *tv)
{
#if TARGET_PLATFORM_IOS || TARGET_PLATFORM_LINUX

    int idx = _buf_idx;
        *tv = _t[idx];
        if(fabs(_cpu_cycle - 0) < 0.0001 && _use_tsc)
        {
            addTimeOffset(*tv, idx);
        }
        else
        {
            TC_Common::gettimeofday(*tv);
        }
#else
    gettimeofday(tv, nullptr);
#endif
}
uint64_t ThreadPool::getNowMs ()
{
    struct timeval tv {};
    getNow(&tv);

    return tv.tv_sec * (int64_t)1000 + tv.tv_usec / 1000;
}
#endif //LINUX_SERVER_LIB_THREAD_MYTHREAD_H_
