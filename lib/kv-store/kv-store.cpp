//
// Created by Yoshiki on 2023/8/3.
//
#include "kv-store.h"
#include "tcp.h"

int main ()
{
    setbuf(stdout, nullptr);
    Tcp <> tcpServer(3000);

    tcpServer.mainLoop();
    return 0;
}