//
// Created by 115282 on 2023/6/1.
//

#include "threadpoolexecutor.h"
#include <sys/sysinfo.h>

#include <algorithm>

uint32_t cpus = get_nprocs_conf();
uint32_t ThreadPoolExecutor::defaultMaxNum = cpus * 10;
uint32_t ThreadPoolExecutor::defaultNum = cpus + 1;

ThreadPoolExecutor::ThreadPoolExecutor (
    uint32_t num,
    uint32_t maxNum,
    const std::chrono::milliseconds &timeout
)
    : Base(new ThreadPoolExecutorPrivate(num, maxNum, timeout, this))
{
    createThreads();
    createTimerObserverThread();
}

ThreadPoolExecutor::ThreadPoolExecutor (ThreadPoolExecutorPrivate *threadPoolExecutorPrivate)
    : Base(threadPoolExecutorPrivate) {}
ThreadPoolExecutor *ThreadPoolExecutor::getInstance (
    uint32_t num,
    uint32_t maxNum,
    const std::chrono::milliseconds &timeout
)
{
    static ThreadPoolExecutor threadPool(num, maxNum, timeout);

    return &threadPool;
}
void ThreadPoolExecutor::stop ()
{
    D_PTR(ThreadPoolExecutor);
    d->~ThreadPoolExecutorPrivate();
}
bool ThreadPoolExecutor::isTerminate () const noexcept
{
    const D_PTR(ThreadPoolExecutor);
    return d->terminate;
}
uint32_t ThreadPoolExecutor::num () const noexcept
{
    const D_PTR(ThreadPoolExecutor);
    return d->num;
}
uint32_t ThreadPoolExecutor::maxNum () const noexcept
{
    const D_PTR(ThreadPoolExecutor);
    return d->maxNum;
}
size_t ThreadPoolExecutor::taskSize () const noexcept
{
    const D_PTR(ThreadPoolExecutor);
    ThreadPoolExecutorPrivate::UniqueLock lock(const_cast<std::mutex &>(d->mutex));
    return d->tasks.size();
}
size_t ThreadPoolExecutor::threadSize () const noexcept
{
    const D_PTR(ThreadPoolExecutor);
    ThreadPoolExecutorPrivate::UniqueLock lock(const_cast<std::mutex &>(d->mutex));
    return d->threads.size();
}
bool ThreadPoolExecutor::createThreads ()
{
    D_PTR(ThreadPoolExecutor);
    for (int i = 0; i < d->num; ++i)
    {
        auto *thread = new Thread(&ThreadPoolExecutor::worker, this);
        if (!thread)
        {
            break;
        }
        thread->detach();
        d->threads.push_back(thread);
    }
    if (d->threads.size() != d->num)
    {
        stop();
        return false;
    }
    return true;
}
void ThreadPoolExecutor::worker ()
{
    D_PTR(ThreadPoolExecutor);
    while (!d->terminate)
    {
        ThreadPoolExecutorPrivate::UniqueLock lock(d->mutex);
        if (d->tasks.empty())
        {
            d->cond.wait(
                lock, [&d] () {
                    return d->terminate || !d->tasks.empty();
                }
            );
        }

        if (d->terminate)
            return;
        // 任务执行完毕标识符，用于taskForAllDown
        d->atomic++;
        if (!d->tasks.empty())
        {
            TaskPtr task = d->tasks.front();
            d->tasks.erase(d->tasks.begin());
            // 此处解锁，避免任务时间过长一直等锁
            lock.unlock();
            task->fun(task);
        }
        d->atomic--;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
void ThreadPoolExecutor::createTimerObserverThread ()
{
    D_PTR(ThreadPoolExecutor);
    if (d->expireTime.count() == 0)
    {
        return;
    }

    d->timerObserver = std::unique_ptr <Thread>(
        new Thread(
            std::bind(
                &ThreadPoolExecutor::timerObserverWorker,
                this
            )
        )
    );
    d->timerObserver->detach();
}
void ThreadPoolExecutor::timerObserverWorker ()
{
    D_PTR(ThreadPoolExecutor);
    while (!d->terminate)
    {
        if (d->tasks.empty())
        {
            std::this_thread::sleep_for(d->expireTime);
            continue;
        }

        // 让出当前cpu，看是否有空闲线程来执行任务
        std::this_thread::yield();

        if (!d->tasks.empty())
        {
            ThreadPoolExecutorPrivate::UniqueLock lock(d->mutex);
            if (!d->tasks.empty())
            {
                auto front = d->tasks.front();
                auto now = getTimeNow();
                auto it = std::max_element(
                    d->tasks.begin(),
                    d->tasks.end(),
                    [&now] (const TaskPtr &res, const TaskPtr &item) {
                        return (now - res->beginTime) < (now - item->beginTime);
                    }
                );

                // 已经等待了多少时间
                auto expireTime = now - (*it)->beginTime;
                if ((expireTime - d->expireTime).count() < 0)
                    d->cond.wait_for(lock, expireTime);
                // 如果等待后还没有线程执行此任务，交由拓展线程执行
                if (!(*it)->isRunning())
                {
                    if (d->extendThread.waitNum.load(std::memory_order_acquire) > 0)
                        d->extendThread.cond.notify_all();
                    else if (d->threads.size() + d->extendThread.threads.size() == d->maxNum)
                        std::this_thread::yield();
                    else
                    {
                        auto *thread = new Thread(
                            &ThreadPoolExecutor::extendWorker,
                            this
                        );
                        thread->detach();
                        d->extendThread.threads.push_back(thread);
                        d->extendThread.waitNum.fetch_add(1, std::memory_order_release);
                    }
                }
            }
        }
    }
}

void ThreadPoolExecutor::extendWorker ()
{
    D_PTR(ThreadPoolExecutor);

    while (!d->terminate)
    {
        ThreadPoolExecutorPrivate::UniqueLock lock(d->mutex);

        if (d->tasks.empty())
        {
            std::cv_status status = d->cond.wait_for(lock, d->expireTime);
            if (status == std::cv_status::timeout || d->terminate)
                return extendWorkerDestroy();
        }
        else
        {
            TaskPtr task = d->tasks.front();
            d->tasks.erase(d->tasks.begin());
            lock.unlock();

            d->extendThread.waitNum.fetch_sub(1, std::memory_order_relaxed);
            d->atomic.fetch_add(1, std::memory_order_relaxed);

            task->fun(task);

            d->atomic.fetch_sub(1, std::memory_order_release);
            d->extendThread.waitNum.fetch_add(1, std::memory_order_release);
        }

    }
}
void ThreadPoolExecutor::extendWorkerDestroy ()
{
    D_PTR(ThreadPoolExecutor);

    d->extendThread.waitNum.fetch_sub(1, std::memory_order_release);
    const std::thread::id pid = std::this_thread::get_id();

    d->extendThread.threads.erase(
        std::find_if(
            d->extendThread.threads.begin(),
            d->extendThread.threads.end(),
            [&pid] (const Thread *v) {
                return pid == v->get_id();
            }
        )
    );
}
bool ThreadPoolExecutor::beforeRunWork (const TaskPtr &task)
{
    return true;
}
void ThreadPoolExecutor::afterRunWork (const ThreadPoolExecutor::TaskPtr &task) {}
bool ThreadPoolExecutor::expireTimeRunWork (const ThreadPoolExecutor::TaskPtr &task)
{
    return true;
}