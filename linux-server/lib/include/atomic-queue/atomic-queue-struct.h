//
// Created by 115282 on 2023/7/28.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
#define LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_

#include "global.h"
template <class T, int N>
class AtomicQueueStruct : BaseTemplate <T>
{
    DECLARE_TEMPLATE_PRIVATE_D(AtomicQueueStruct, T##N);
};
#endif //LINUX_SERVER_LIB_INCLUDE_ATOMIC_QUEUE_ATOMIC_QUEUE_STRUCT_H_
