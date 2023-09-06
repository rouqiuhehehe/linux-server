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
    // HashTable <std::string, int> cc;
    // cc.emplace("dsa", 1123);
    // cc.emplace("cdsa", 11423);
    //
    // std::unordered_map <int, int> sd;
    //
    // auto end = cc.end();
    // for (int i = 0; i < 1000; ++i)
    // {
    //     if (cc.emplace(std::to_string(i), i).second == end)
    //         std::cout << i << std::endl;
    // }
    // HashTable <std::string, int>::Iterator it = cc.begin();
    // do
    // {
    //     it = cc.erase(it).second;
    // } while (it);
    setbuf(stdout, nullptr);
    Tcp <> tcpServer(3000);

    tcpServer.mainLoop();
    return 0;
}