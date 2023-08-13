//
// Created by Yoshiki on 2023/8/13.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
#include "kv-string.h"

#define FIND_COMMAND                                                        \
        static inline Commands findCommand (const std::string &command)     \
        {                                                                   \
            int cmd = static_cast<int>(Commands::NIL);                      \
            for (; cmd != static_cast<int>(Commands::END); ++cmd)           \
            {                                                               \
                if (std::strcmp(command.c_str(), commands[cmd]) == 0)       \
                break;                                                      \
            }                                                               \
                                                                            \
            return Commands(cmd);                                           \
        }

class BaseCommand
{
protected:
    enum class Commands { NIL = 0, FLUSHALL, DEL, EXPIRE, KEYS, FLUSHDB, EXISTS, END };
    enum class StructType { NIL = 0, STRING, LIST, HASH, SET, ZSET, END };

    virtual void handlerFlushAll (const CommandParams &commandParams) = 0;
    virtual void handlerDel (const CommandParams &commandParams) = 0;
    virtual void handlerExpire (const CommandParams &commandParams) = 0;
    virtual void handlerKeys (const CommandParams &commandParams) = 0;
    virtual void handlerFlushDb (const CommandParams &commandParams) = 0;
    virtual void handlerExists (const CommandParams &commandParams) = 0;
    // 检查当前key是否是当前操作的数据结构
    // ex: lpush 不能操作 set 的key
    inline bool checkKeyType (const CommandParams &commandParams, StructType structType)
    {
        auto it = keyOfStructType.find(commandParams.key);
        if (it != keyOfStructType.end() && it->second != structType)
            return false;
        return true;
    }

    inline ValueType handlerCommand (Commands cmd, const CommandParams &commandParams)
    {
        switch (cmd)
        {
            case Commands::FLUSHALL:
                handlerFlushAll(commandParams);
            case Commands::DEL:
                break;
            case Commands::EXPIRE:
                break;
            case Commands::KEYS:
                break;
            case Commands::FLUSHDB:
                break;
            case Commands::EXISTS:
                break;
            case Commands::NIL:
                break;
        }
    }
    // 设置key的时候插入keyOfStructType查找对应的type
    static inline StructType findStructType (const std::string &structType)
    {
        int cmd = static_cast<int>(StructType::NIL);
        for (; cmd != static_cast<int>(StructType::END); ++cmd)
        {
            if (std::strcmp(structType.c_str(), structTypes[cmd]) == 0)
                break;
        }

        return StructType(cmd);
    }
    FIND_COMMAND

private:
    std::unordered_map <std::string, StructType> keyOfStructType;
    static const char *commands[];
    static const char *structTypes[];
};
class StringCommand
{
public:
    enum Commands { NIL, SET, GET, INCR, INCRBY, DECR, DECRBY, APPEND, GETSET, END };
    FIND_COMMAND
private:
    static const char *commands[];
};

class Command : public BaseCommand
{
    void handlerFlushAll (const CommandParams &commandParams) override {}
    void handlerDel (const CommandParams &commandParams) override {}
    void handlerExists (const CommandParams &commandParams) override {}
    void handlerExpire (const CommandParams &commandParams) override {}
    void handlerFlushDb (const CommandParams &commandParams) override {}
    void handlerKeys (const CommandParams &commandParams) override {}

    CommandParams splitCommandParams (std::string &&str)
    {
        auto commandVector = Utils::stringSplit(str, ' ');
        auto command = commandVector[0];
        auto key = commandVector[1];
        commandVector.erase(commandVector.begin(), commandVector.begin() + 2);

        CommandParams commandParams = {
            .command = command,
            .key = key,
            .params = std::move(commandVector)
        };

        handlerCommand(commandParams);
    }

private:
    ValueType handlerCommand (const CommandParams &commandParams)
    {
        Commands command = findCommand(commandParams.command);
        if (command != Commands::END)
        {
            return handlerCommand(commandParams);
        }

        StringCommand::Commands stringCommand = StringCommand::findCommand(commandParams.command);
        if (stringCommand != StringCommand::Commands::END)
        {

        }
    }

    StringCommand stringCommands;
};

const char *BaseCommand::commands[] { "FLUSHALLL", "DEL", "EXPIRE", "KEYS", "FLUSHDB", "EXISTS" };
const char *BaseCommand::structTypes[] { "STRING", "LIST", "HASH", "SET", "ZSET" };
const char *StringCommand::commands[]
    { "SET", "GET", "INCR", "INCRBY", "DECR", "DECRBY", "APPEND", "GETSET" };
#endif //LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
