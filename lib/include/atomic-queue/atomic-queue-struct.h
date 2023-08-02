//
// Created by 115282 on 2023/7/28.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
#define LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_

#include "global.h"
#include <list>
#include <atomic>
#include "util.h"
#include <thread>

#define ATOMIC_QUEUE_STRUCT_DEFAULT_SIZE (1 << 16)

template <class T, int N = ATOMIC_QUEUE_STRUCT_DEFAULT_SIZE>
class AtomicQueueStruct;
template <class T, int N>
class AtomicQueueStructPrivate
{
    DECLARE_TEMPLATE_2_PUBLIC_Q(AtomicQueueStruct, T, N);
public:
    virtual ~AtomicQueueStructPrivate () noexcept = default;
protected:
    AtomicQueueStructPrivate () = default;

    T dataRing[N];
    std::atomic <size_t> readIndex { 0 };
    std::atomic <size_t> writeIndex { 0 };
    // 最后一个可读元素下标，如果和writeIndex -1 不一致，说明有写请求成功写数据，但还未入队列
    std::atomic <size_t> maxReadIndex { 0 };
};
template <class T, int N>
class AtomicQueueStruct : Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_2_PRIVATE_D(AtomicQueueStruct, T, N);
public:
    AtomicQueueStruct ()
        : d_ptr(new AtomicQueueStructPrivate <T, N>) {}
    ~AtomicQueueStruct () noexcept override = default;

    template <class ...Arg>
    explicit AtomicQueueStruct (const Arg &...item)
        : d_ptr(new AtomicQueueStructPrivate <T, N>) { enqueue(std::forward <Arg>(item)...); }

    inline size_t size () const noexcept
    {
        const D_TEMPLATE_2_PTR(AtomicQueueStruct, T, N);
        size_t writeIndex = atomicLoadRelaxed(d->writeIndex);
        size_t readIndex = atomicLoadRelaxed(d->readIndex);
        if (writeIndex >= readIndex)
            return writeIndex - readIndex;
        else
            return N + writeIndex - readIndex;
    }

    template <class R>
    inline bool enqueue (R &&item)
    {
        return enqueue_(std::forward <R>(item));
    }

    template <class R, class ...Arg>
    inline bool enqueue (R &&item, Arg &&...items)
    {
        if (!enqueue_(std::forward <R>(item)))
            return false;

        return enqueue(std::forward <Arg>(item)...);
    }
    // bool dequeue ();
    // bool tryDequeue ();
protected:
    DECLARE_TEMPLATE_2_PTR_D(AtomicQueueStruct, T, N);
    explicit AtomicQueueStruct (AtomicQueueStructPrivate <T, N> *d)
        : d_ptr(d) {}

    inline static size_t atomicLoadRelaxed (const std::atomic <size_t> &atomic) noexcept
    {
        return atomic.load(std::memory_order_relaxed);
    }

    template <class R>
    bool enqueue_ (R &&item)
    {
        D_TEMPLATE_2_PTR(AtomicQueueStruct, T, N);
        size_t currentWriteIndex;
        size_t currentReadIndex;

        do
        {
            currentWriteIndex = atomicLoadRelaxed(d->writeIndex);
            currentReadIndex = atomicLoadRelaxed(d->readIndex);
            // 队列满了
            if (currentWriteIndex + 1 == currentReadIndex)
                return false;

            if (unlikely(currentWriteIndex == N - 1))
                currentWriteIndex = 0;
        } while (
            !d->writeIndex.compare_exchange_weak(
                currentWriteIndex,
                currentWriteIndex + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed
            ));

        d->dataRing[currentWriteIndex] = std::forward <R>(item);

        // 只是用于交换，compare_exchange_weak失败时会将当前值写到compareWriteIndex中
        // 存在多线程操作，让maxReadIndex按顺序写入
        size_t compareWriteIndex = currentWriteIndex;
        while (!d->maxReadIndex
                 .compare_exchange_weak(
                     compareWriteIndex,
                     currentWriteIndex + 1,
                     std::memory_order_acq_rel,
                     std::memory_order_relaxed
                 ))
        {
            compareWriteIndex = currentWriteIndex;
            std::this_thread::yield();
        }

        return true;
    }
};
#endif //LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
