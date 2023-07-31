//
// Created by 115282 on 2023/7/28.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
#define LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_

#include "global.h"
#include <list>
#include <atomic>
#include "util.h"

template <class T, int N>
class AtomicQueueStruct;
template <class T, int N>
class AtomicQueueStructPrivate
{
    class ChuckList
    {
        class Chuck
        {
            T value[N];
        };

        inline void push ()
        {
            if (++endPos != N)
                return;

            while (!lock)
            {
                lock = true;
                chuckList.emplace_back({});
                endPos = 0;
            }
        }

        inline void pop ()
        {

        }
        std::list <Chuck> chuckList;
        int endPos = 0;
        std::atomic <bool> lock { false };
    };

    ChuckList chuck;
};
template <class T, int N>
class AtomicQueueStruct : Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_2_PRIVATE_D(AtomicQueueStruct, T, N);
public:
    AtomicQueueStruct ()
        : d_ptr(new AtomicQueueStructPrivate <T, N>) {}

    template <class ...Arg>
    explicit AtomicQueueStruct (const Arg &...item)
    {

    }

    explicit AtomicQueueStruct (const T &item)
    {
        push(item);
    }

    void push (const T &item)
    {
        D_TEMPLATE_2_PTR(AtomicQueueStruct, T, N);
        if (++d->endPos != N)
        {

        }
    }
protected:
    DECLARE_TEMPLATE_2_PTR_D(AtomicQueueStruct, T, N);
};
#endif //LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
