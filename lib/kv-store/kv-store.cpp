//
// Created by Yoshiki on 2023/8/3.
//
#include "kv-store.h"
#include <list>
#include <set>
#include <chrono>
#include "tcp.h"

int main ()
{
    std::list <int> aa;
    SkipList <int> c;
    auto a = c.begin();
    auto d = c.end();
    std::set <int> ff;

    auto begin = std::chrono::system_clock::now();
    for (int i = 0; i < 100; ++i)
    {
        c.insert(i);
        // c.insert(i);
        // std::cout << i << std::endl;
    }
    auto end = std::chrono::system_clock::now();

    std::cout << std::chrono::duration_cast <std::chrono::milliseconds>(end - begin).count()
              << std::endl;

    std::cout << c.erase(99);
    auto it = c.find(66);
    if (it)
        std::cout << *it << std::endl;
    else
        std::cout << "not find" << std::endl;
    // c.insert(1);
    // c.insert(2);
    // auto dd = c.find(51);
    // std::cout << *dd << std::endl;

    return 0;
}