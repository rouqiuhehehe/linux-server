//
// Created by Administrator on 2023/4/25.
//
#include <atomic>
#include <mutex>

#define USE_FENCE
class Singleton
{
public:
    Singleton (const Singleton &) = delete;
    Singleton &operator= (const Singleton &) = delete;

    static Singleton *getInstance ()
    {
#ifdef USE_FENCE
        auto tmp = instance.load(std::memory_order_relaxed);
        // 此指令后面的指令不允许重排到此指令之前
        std::atomic_thread_fence(std::memory_order_acquire);
#else
        auto tmp = instance.load(std::memory_order_acquire);
#endif

        if (tmp == nullptr)
        {
#ifdef USE_FENCE
            tmp = instance.load(std::memory_order_relaxed);
#else
            tmp = instance.load(std::memory_order_acquire);
#endif
            if (tmp == nullptr)
            {
                std::lock_guard <std::mutex> lock(mutex);
                tmp = new Singleton;

#ifdef USE_FENCE
                // 此指令前面的指令不允许重排到此指令之后
                std::atomic_thread_fence(std::memory_order_release);
                instance.store(tmp, std::memory_order_relaxed);
#else
                instance.store(tmp, std::memory_order_release);
#endif
                // 程序退出的时候执行
                atexit(destructor);
            }
        }
        return tmp;
    }

private:
    Singleton () = default;
    ~Singleton () = default;

    static void destructor ()
    {
        auto tmp = instance.load(std::memory_order_relaxed);

        delete tmp;
    }

    // 此处不能用volatile
    // volatile 只作用在编译器上，并不能阻止cpu的乱序执行
    static std::atomic <Singleton *> instance;
    static std::mutex mutex;
};
std::atomic <Singleton *> Singleton::instance;
std::mutex Singleton::mutex;

int main ()
{

}