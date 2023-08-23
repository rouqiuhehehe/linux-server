//
// Created by Yoshiki on 2023/8/23.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_HASH_COMMAND_H_
#define LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_HASH_COMMAND_H_

#include "command-common.h"

class HashCommandHandler : public CommandCommon
{
public:
    enum Commands {
        NIL = -1,
        HSET,
        HGET,
        HGETALL,

    };
};
#endif //LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_HASH_COMMAND_H_
