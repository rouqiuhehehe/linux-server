//
// Created by 115282 on 2023/7/25.
//

#include <queue>
#include <iostream>
#include "timeout.h"

int main ()
{
    Timeout <> timeout;
    timeout.setInterval(
        [] {
            std::cout << 2000 << std::endl;
        },
        std::chrono::milliseconds(2000)
    );
    timeout.setTimeout(
        [] {
            std::cout << "5s" << std::endl;
        }, std::chrono::seconds(5));
    timeout.setInterval(
        [] {
            std::cout << 1000 << std::endl;
        },
        std::chrono::milliseconds(1000)
    );

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}