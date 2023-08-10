//
// Created by Yoshiki on 2023/8/3.
//
#include "kv-store.h"
#include <list>
#include <set>
#include <chrono>
#include "tcp.h"
#include <algorithm>
#include "logger.h"

int main ()
{
    Logger logger("../log/");
    LOG_WARING("{0}+{1}={2}", 1, 2, 1 + 2);
    spdlog::default_logger()->flush();
    MainReactor <4> reactor(3);
    reactor.mainLoop();

    return 0;
}