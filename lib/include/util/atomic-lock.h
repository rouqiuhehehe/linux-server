//
// Created by 115282 on 2023/9/11.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_UTIL_ATOMIC_LOCK_H_
#define LINUX_SERVER_LIB_INCLUDE_UTIL_ATOMIC_LOCK_H_

#include <atomic>
namespace Utils
{
    class AtomicLock
    {
    public:
        void lock (std::memory_order m = std::memory_order_acq_rel)
        {
            while (lock_.test_and_set(m));
        }
        bool tryLock (std::memory_order m = std::memory_order_acq_rel)
        {
            return lock_.test_and_set(m);
        }
        void unlock (std::memory_order m = std::memory_order_release)
        {
            lock_.clear(m);
        }

    private:
        std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
    };
}
#endif //LINUX_SERVER_LIB_INCLUDE_UTIL_ATOMIC_LOCK_H_
