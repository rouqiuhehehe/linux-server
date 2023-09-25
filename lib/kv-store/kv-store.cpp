//
// Created by Yoshiki on 2023/8/3.
//
#include "util/random.h"
#include "kv-store.h"

int main ()
{
    // std::vector <int> a { 1, 2, 3, 4, 5 };
    // std::vector <int> b;
    //
    // std::string ad = "123";
    // ad.clear();
    //
    // std::move(a.begin(), a.end(), std::back_inserter(b));
    setbuf(stdout, 0);
    Tcp <> tcpServer(3000);
    tcpServer.mainLoop();

    return 0;
}