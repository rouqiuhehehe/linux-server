//
// Created by Yoshiki on 2023/8/12.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_STRING_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_STRING_H_

#include <map>
#include "data-structure/kv-skiplist.h"
#include <unordered_map>
#include "kv-value.h"

// SET GET INCR INCRBY DECR DECRBY APPEND GETSET
class KvStringStruct
{
public:

private:
    std::unordered_map<KeyType, StringValueType> stringMap;
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_STRING_H_
