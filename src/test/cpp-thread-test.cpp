//
// Created by Administrator on 2023/4/27.
//

#include <future>
#include <iostream>
#include <memory>
uint64_t sleep (uint64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return ms;
};

int main ()
{
    {
        // 开启新线程
        // auto ret = std::async(std::launch::async, sleep, 5000);
        auto ret = std::async(std::launch::async, sleep, 5000);
    }

    {
        std::packaged_task <decltype(sleep)> packagedTask(sleep);
        auto task = packagedTask.get_future();

        packagedTask(1000);

        std::cout << task.get() << std::endl;
    }

    {
        auto task = [] (std::future <int> i) {
            std::cout << i.get() << std::endl;
        };

        std::promise <int> p;
        std::thread t(std::move(task), p.get_future());
        t.detach();

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        p.set_value(20);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}