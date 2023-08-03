//
// Created by 115282 on 2023/7/28.
//
#include "atomic-queue/atomic-queue-struct.h"

#define PRODUCER_SIZE 4
#define CONSUMER_SIZE 5

class A
{
public:
    A () = default;
    A (const A &)
    {
        std::cout << "A & COPY" << std::endl;
    }
    A (A &&) noexcept
    {
        std::cout << "A && copy" << std::endl;
    }
    A &operator= (const A &)
    {
        std::cout << "&" << std::endl;
        return *this;
    }
    A &operator= (A &&) noexcept
    {
        std::cout << "&&" << std::endl;
        return *this;
    }

    friend std::ostream &operator<< (std::ostream &os, const A &)
    {
        os << "dddddddddd" << std::endl;
        return os;
    }
};

int main ()
{
    std::thread producer[PRODUCER_SIZE];
    std::thread consumer[CONSUMER_SIZE];
    AtomicQueueStruct <std::string> c;
    volatile bool terminate = false;

    for (auto &i : consumer)
        i = std::move(
            std::thread(
                [&c, &terminate] () {
                    std::string str;
                    while (!terminate)
                    {
                        auto v = c.tryDequeue(str);
                        if (v)
                            std::cout << std::this_thread::get_id() << " get value : " << str
                                      << std::endl;
                        else
                            std::cout << std::boolalpha << v << std::endl;

                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
            ));

    for (auto &i: producer)
        i = std::move(
            std::thread(
                [&c, &terminate] () {
                    while (!terminate)
                    {
                        std::string str = std::move(Utils::getRandomChinese());
                        c.enqueue(str);
                        std::cout << std::this_thread::get_id() << " set value : " << str
                                  << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
            ));

    std::this_thread::sleep_for(std::chrono::seconds(10));
    terminate = true;
    for (auto &i: consumer)
        i.join();
    for (auto &i : producer)
        i.join();

    return 0;
}