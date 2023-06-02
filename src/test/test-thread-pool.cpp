//
// Created by Administrator on 2023/4/27.
//
#include "threadpool.h"
#include <iostream>

class MyThreadPool : public ThreadPool
{
protected:
    bool beforeRunTask (const TaskFunPtr &task) override
    {
        std::cout << (task->expireTime() - getNowMs()) << std::endl;

        return true;
    }
    void afterRunTask (const ThreadPool::TaskFunPtr &task) override
    {
        std::cout << task->expireTime() << std::endl;
    }

};
int main ()
{
    MyThreadPool threadPool;

    threadPool.start();
    auto c = threadPool.execf(
        1000, std::bind(
            [] (int a) {
                std::cout << a << std::endl;

                return 20;
            }, 1
        ));
    threadPool.waitForAllDone();
    threadPool.stop();

    std::cout << c.get() << std::endl;
    return 0;
}