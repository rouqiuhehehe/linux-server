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

template <class Fn, class ...Arg>
void compTime (Fn &&fn, Arg &&... arg)
{
    using clock = std::chrono::high_resolution_clock;
    auto begin = clock::now();
    fn(std::forward <Arg>(arg)...);
    auto end = clock::now();

    std::cout << "time use : "
              << std::chrono::duration_cast <std::chrono::milliseconds>(end - begin).count()
              << std::endl;
}
int main ()
{
    {
        // 开启新线程
        // auto ret = std::async(std::launch::async, sleep, 5000);
        compTime(
            [] () {
                auto ret = std::async(std::launch::async, sleep, 5000);
            }
        );
    }

    {
        compTime(
            [] () {
                std::packaged_task <decltype(sleep)> packagedTask(sleep);
                auto task = packagedTask.get_future();

                packagedTask(1000);

                std::cout << task.get() << std::endl;
            }
        );
    }

    {
        compTime(
            [] () {
                auto task = [] (std::future <int> i) {
                    std::cout << i.get() << std::endl;
                };

                std::promise <int> p;
                std::thread t(std::move(task), p.get_future());
                t.detach();

                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                p.set_value(20);
            }
        );
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}