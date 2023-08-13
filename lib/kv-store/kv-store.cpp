//
// Created by Yoshiki on 2023/8/3.
//
#include "kv-store.h"
#include <list>
#include <set>
#include <chrono>
#include "tcp.h"
#include <algorithm>

int main ()
{
    // setbuf(stdout, nullptr);
    // Tcp <> tcpServer(3000);
    //
    // tcpServer.mainLoop();
    std::string aa { "set ddd            dsa         " };
    auto res = Utils::stringSplit(aa, ' ', true);

    std::chrono::milliseconds a { -1 };
    return 0;
}