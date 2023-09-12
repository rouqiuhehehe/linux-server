//
// Created by 115282 on 2023/9/12.
//

#include <concepts>
#include "reflect-helper.h"
#include <iostream>
template <class T>
concept MyRules =
requires (T t) {
    std::default_initializable <T>;
    static_cast<int>(t);
};
class A
{
public:
    A ();
    explicit operator int ()
    {
        return 1;
    }
};

struct B
{
    int a;
    int v;
    int s;
};

template <MyRules T>
T fun ()
{
    static T t {};
    return t;
}

int main ()
{
    // auto c = fun <A>();
    auto cd = CountMember <B>();
    std::cout << cd << std::endl;
}