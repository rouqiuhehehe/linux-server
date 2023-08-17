//
// Created by Yoshiki on 2023/8/13.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
#include <algorithm>
#include "util/string-helper.h"
#include "util/math-helper.h"
#include "kv-value.h"
#include "unordered_map"
#include "command-structs/command-common.h"
#include "command-structs/kv-string-command.h"

class BaseCommandHandler
{
protected:
    enum class Commands { NIL = -1, FLUSHALL, DEL, EXPIRE, KEYS, FLUSHDB, EXISTS, TYPE, END };
    enum class StructType { NIL = -1, STRING, LIST, HASH, SET, ZSET, END };

    virtual void handlerFlushAll (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerDel (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerExpire (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerKeys (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerFlushDb (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerExists (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerType (const CommandParams &commandParams, ResValueType &resValue) = 0;

    inline ResValueType handlerCommand (const CommandParams &commandParams, Commands cmd)
    {
        ResValueType resValue;
        switch (cmd)
        {
            case Commands::FLUSHALL:
                handlerFlushAll(commandParams, resValue);
                break;
            case Commands::DEL:
                handlerDel(commandParams, resValue);
                break;
            case Commands::EXPIRE:
                handlerExpire(commandParams, resValue);
                break;
            case Commands::KEYS:
                handlerKeys(commandParams, resValue);
                break;
            case Commands::FLUSHDB:
                handlerFlushDb(commandParams, resValue);
                break;
            case Commands::EXISTS:
                handlerExists(commandParams, resValue);
                break;
            case Commands::TYPE:
                handlerType(commandParams, resValue);
                break;
            case Commands::NIL:
            case Commands::END:
                break;
        }

        return resValue;
    }

    // 检查当前key是否是当前操作的数据结构
    // ex: lpush 不能操作 set 的key
    inline bool checkKeyType (
        const CommandParams &commandParams,
        StructType structType,
        ResValueType &res
    )
    {
        auto it = keyOfStructType.find(commandParams.key);
        if (it != keyOfStructType.end() && it->second != structType)
        {
            res.model = ResValueType::ReplyModel::REPLY_ERROR;
            res.setErrorStr(commandParams, ResValueType::ErrorType::WRONGTYPE);
            return false;
        }
        return true;
    }

    FIND_COMMAND

protected:
    std::unordered_map <std::string, StructType> keyOfStructType;
    static const char *commands[];
};

class CommandHandler final : public BaseCommandHandler
{

public:
    /*
     * 入口函数，处理命令
     * @param params 处理后的命令
     * @return ResValueType
     */
    ResValueType handlerCommand (const CommandParams &commandParams)
    {
        Commands command = findCommand(commandParams.command);
        ResValueType res;
        bool isNewKey = false;
        if (command != Commands::END)
        {
            return BaseCommandHandler::handlerCommand(commandParams, command);
        }

        StringCommandHandler::Commands
            StringCommand = StringCommandHandler::findCommand(commandParams.command);

        if (StringCommand != StringCommandHandler::Commands::END)
        {
            if (!checkKeyType(commandParams, StructType::STRING, res))
                return res;

            res = std::move(
                stringCommandHandler.handlerCommand(
                    commandParams,
                    StringCommand,
                    isNewKey
                ));
            if (isNewKey)
                keyOfStructType.emplace(commandParams.key, StructType::STRING);

            return res;
        }

        res.setErrorStr(commandParams, ResValueType::ErrorType::UNKNOWN_COMMAND);
        return res;
    }
    static CommandParams splitCommandParams (const std::string &str)
    {
        auto commandVector = Utils::StringHelper::stringSplit(str, ' ');
        auto command = commandVector[0];
        // 转成小写
        Utils::StringHelper::stringTolower(command);

        std::string key;
        if (commandVector.size() > 1)
        {
            key = commandVector[1];
            commandVector.erase(commandVector.begin(), commandVector.begin() + 2);
        }
        else
            commandVector.clear();

        return {
            .command = command,
            .key = key,
            .params = std::move(commandVector)
        };
    }
protected:
    void handlerFlushAll (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerDel (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerExpire (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerKeys (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerFlushDb (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerExists (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }
    void handlerType (const CommandParams &commandParams, ResValueType &resValue) override
    {

    }

private:
    StringCommandHandler stringCommandHandler;
};

const char
    *BaseCommandHandler::commands[]
    { "flushall", "del", "expire", "keys", "flushdb", "exists", "type" };

#endif //LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
