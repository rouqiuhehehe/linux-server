//
// Created by Administrator on 2023/4/25.
//
#include <iostream>
template <class T>
class Singleton
{
public:
    Singleton (const Singleton &) = delete;
    Singleton &operator= (const Singleton &) = delete;

    static T *getInstance ()
    {
        // 利用局部静态变量特性
        // 延迟加载，自动回收内存，自动调用析构函数
        // 不使用new，避免编译器优化指令导致的重排
        // c++ static局部变量是线程安全的
        static T instance;

        return &instance;
    }

protected:
    Singleton () = default;
    virtual ~Singleton ()
    {
        std::cout << "调用析构函数" << std::endl;
    }
};

class Instance : public Singleton <Instance>
{
public:
    int a () const // NOLINT(readability-convert-member-functions-to-static)
    {
        return 1;
    }
};
int main ()
{
    auto instance = Instance::getInstance();

    std::cout << instance->a() << std::endl;
}