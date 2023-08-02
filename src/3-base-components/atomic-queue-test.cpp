//
// Created by 115282 on 2023/7/28.
//
#include "atomic-queue/atomic-queue-struct.h"

class A
{
public:
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
};
int main ()
{
    AtomicQueueStruct <A> c;
    A a, b, d, e, f;

    c.enqueue(std::move(a), b, d, e, std::move(f));
    return 0;
}