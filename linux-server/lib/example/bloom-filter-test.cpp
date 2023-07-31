//
// Created by Administrator on 2023/4/23.
//

#include "bloom-filter.h"

#include <iostream>
#include <string>
#include <list>

int main ()
{

    BloomFilter <1000> a;

    std::list <std::string> list = { "abc", "dsc", "eeee" };
    for (const auto &v : list)
        a.insert(v);

    for (const auto &v : list)
        if (a.contains(v))
            std::cout << v << " in bloom filter\n";

    a.containsAll(list.begin(), list.end());
    return 0;
}