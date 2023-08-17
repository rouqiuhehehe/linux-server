//
// Created by 115282 on 2023/8/17.
//

#define _GNU_SOURCE
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

class A
{
public:
    virtual void b ()
    {
        std::cout << "bbb" << std::endl;
    }
};

void bb (A *a)
{
    std::cout << "dsad" << std::endl;
    a->b();
}

using bType = void (*) (A *);
int main ()
{
    A a;

    auto *fn = reinterpret_cast<intptr_t *>(*reinterpret_cast<intptr_t *>(&a));
    size_t pageSize = getpagesize();
    auto ret = mprotect((intptr_t *)((intptr_t)fn & ~(pageSize - 1)),
        pageSize,
        PROT_WRITE | PROT_READ | PROT_EXEC
    );
    memcpy(fn, reinterpret_cast<intptr_t *>(&bb), 8);
    // reinterpret_cast<void (*) (A *)>(*reinterpret_cast<intptr_t *>(*reinterpret_cast<intptr_t *>(&a)))(
    //     &a
    // );

    a.b();
    return 0;
}