//
// Created by Administrator on 2023/4/24.
//
#include "template-fun.h"
#include <string>
#include <iostream>
class A
{
public:
    std::string str (int a, int b, int c)
    {
        return "!23";
    }
};
int main ()
{
    std::cout << CheckStr <A, std::string, int, double>::ret << std::endl;

    return 0;
}