//
// Created by Administrator on 2023/4/27.
//

#include "threadpool.h"

ThreadPool::ThreadPool (int num)
    : threadNum_(num) {}

ThreadPool::~ThreadPool ()
{
    stop();
}
void ThreadPool::stop ()
{
    {
        // 加单独作用域  在notify_all切换线程的时候释放锁
        UniqueLock lock(mutex_);
        terminate_ = true;

        condition_.notify_all();
    }

    for (auto &thread : threads_)
    {
        if (thread->joinable())
            // 如果线程可以执行，执行线程等到线程退出
            thread->join();

        delete thread;
        thread = nullptr;
    }

    UniqueLock lock(mutex_);
    threads_.clear();
}
bool ThreadPool::start ()
{
    UniqueLock lock(mutex_);

    if (!threads_.empty())
        return false;

    for (int i = 0; i < threadNum_; ++i)
    {
        auto *p = new std::thread(&ThreadPool::run, this);
        p->detach();
        threads_.push_back(p);
    }

    return true;
}
void ThreadPool::run ()
{
    while (!isTerminate())
    {
        auto task = get();
        if (task)
        {
            ++atomic_;

            try
            {
                if (task->expireTime_ != 0 && task->expireTime_ < getNowMs())
                    // 超时处理
                    expireTaskHandler(task);
                else
                    task->func_();
            }
            catch (...)
            {
                errorHandler();
            }

            --atomic_;

            UniqueLock lock(mutex_);
            // 如果所有任务都执行完毕
            // atomic_为了防止任务被pop后 没有执行完
            if (atomic_ == 0 && tasks_.empty())
                condition_.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
ThreadPool::TaskFunPtr ThreadPool::get ()
{
    UniqueLock lock(mutex_);

    if (tasks_.empty())
    {
        condition_.wait(
            lock, [this] () {
                return terminate_ || !tasks_.empty();
            }
        );
    }

    if (terminate_)
        return nullptr;

    if (!tasks_.empty())
    {
        auto v = tasks_.front();
        tasks_.pop();

        return v;
    }

    return nullptr;
}
void ThreadPool::waitForAllDone (int millSecond)
{
    UniqueLock lock(mutex_);

    if (tasks_.empty()) return;

    auto cb = [this] () { return tasks_.empty(); };
    if (millSecond <= 0)
        condition_.wait(lock, cb);
    else
        condition_.wait_for(lock, std::chrono::milliseconds(millSecond), cb);
}
