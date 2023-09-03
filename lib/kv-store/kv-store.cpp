//
// Created by Yoshiki on 2023/8/3.
//
#include "data-structure/kv-incrementally-hash.h"
#include "kv-store.h"
#include "kv-tcp.h"

class A
{
public:
    A (int *a)
        : a(a) {}
    A &operator++ ()
    {
        a = 0;
    }
    A operator++ (int)
    {
        auto old = *this;
        ++(*this);
        return old;
    }
private:
    int *a;
};
int main ()
{
    // auto c = df.begin();
    HashTable <std::string, int> cc;
    cc.emplace("dsa", 1123);

    std::unordered_map <int, int> sd;
    cc.erase("dsa");
    // setbuf(stdout, nullptr);
    // Tcp <> tcpServer(3000);
    //
    //
    // tcpServer.mainLoop();
    return 0;
}