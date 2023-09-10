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
class B
{
public:
    void a ()
    {

    }
protected:
    int c;
};
class C : public B
{
    void a ()
    {
        c = 10;
    }
};
int main ()
{
    // auto c = df.begin();
    HashTable <std::string, int> cc;
    cc.emplace("dsa", 1123);
    cc.emplace("cdsa", 11423);

    std::unordered_map <int, int> sd;
    sd.emplace(123, 324);
    sd.emplace(1234, 3244);
    sd.emplace(12344, 32444);
    sd.emplace(12355, 32455);
    std::unordered_map <int, int> sdd = sd;

    IncrementallyHashTable <int, int> d;
    IncrementallyHashTable <int, int>::_IncrementallyHashTableIterator cdc;
    ++cdc;

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
    // Tcp <> tcpServer(3000);

    HashTable <std::string, int> dd = cc;
    for (auto &v : cc)
    {

    }

    // tcpServer.mainLoop();
    return 0;
}