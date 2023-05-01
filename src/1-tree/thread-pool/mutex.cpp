#include <iostream>       // std::cout
#include <thread>         // std::thread-pool
#include <mutex>          // std::mutex
#include <future>

volatile int counter(0); // non-atomic counter
std::mutex mtx;           // locks access to counter

void increases_10k (int idx)
{
    if (idx == 0)
        std::cout << "thread-pool run\t" << std::this_thread::get_id() << std::endl;
    // if (mtx.try_lock())
    for (int i = 0; i < 10000; ++i)
        // 1. 使用try_lock的情况
        // ++counter;
        // mtx.unlock();
        // 2. 使用lock的情况
    {
        if (mtx.try_lock())
        {
            // mtx.lock();
            ++counter;
            mtx.unlock();
        }
    }
}

int main ()
{
    std::terminate();
    std::thread threads[10];
    int idx = 0;
    for (auto &i : threads)
        i = std::thread(increases_10k, idx++);

    std::cout << threads[0].get_id() << std::endl;
    for (auto &th : threads) th.join();
    std::cout << " successful increases of the counter " << counter << std::endl;

    return 0;
}