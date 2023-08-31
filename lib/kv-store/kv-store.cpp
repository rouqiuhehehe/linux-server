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
    auto c = new int { 3 };
    A a(c);
    auto d = a++;
    // std::unordered_map <int, A> df;
    // auto c = df.begin();
    HashTable <std::string, int> cc;

    cc.reHash(10);
    // setbuf(stdout, nullptr);
    // Tcp <> tcpServer(3000);
    //
    //
    // tcpServer.mainLoop();
    return 0;
}