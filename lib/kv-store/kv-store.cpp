//
// Created by Yoshiki on 2023/8/3.
//
#include "kv-store.h"

class A
{
public:
    A (const A &);
    A (int);
};
class B : public A
{
    using A::A;
    B (const B &);
};
int main ()
{
    // auto c = df.begin();
    HashTable <std::string, int> cc;
    cc.emplace("dsa", 1123);
    cc.emplace("cdsa", 11423);

    // setbuf(stdout, nullptr);
    // Tcp <> tcpServer(3000);
    IncrementallyHashTable <int, int> df;
    for (int i = 0; i < 20; ++i)
    {
        df.emplace(i, i);
    }

    df.erase(5);
    df.erase(3);
    auto dd = df.find(1);

    for (const auto &v: df)
    {
        std::cout << "dddd : " << v.first << std::endl;
    }

    // tcpServer.mainLoop();
    return 0;
}