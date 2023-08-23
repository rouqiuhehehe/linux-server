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
#include "command-structs/kv-hash-command.h"

class BaseCommandHandler
{
protected:
    using ExpireMapType = std::unordered_map <KeyType,
                                              std::pair <StructType, std::chrono::milliseconds>>;
    using AllKeyMapType = std::unordered_map <KeyType, StructType>;
    enum class Commands
    {
        NIL = -1,
        FLUSHALL,
        DEL,
        EXPIRE,
        PEXPIRE,
        TTL,
        PTTL,
        KEYS,
        FLUSHDB,
        EXISTS,
        TYPE,
        END
    };

    virtual void handlerFlushAll (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerDel (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerExpire (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerPExpire (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerTTL (const CommandParams &commandParams, ResValueType &resValue) = 0;
    virtual void handlerPTTL (const CommandParams &commandParams, ResValueType &resValue) = 0;
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
            case Commands::PEXPIRE:
                handlerPExpire(commandParams, resValue);
                break;
            case Commands::TTL:
                handlerTTL(commandParams, resValue);
                break;
            case Commands::PTTL:
                handlerPTTL(commandParams, resValue);
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
    AllKeyMapType keyOfStructType;
    ExpireMapType expireKey;
    static const char *commands[];
};

class CommandHandler final : public BaseCommandHandler, public CommandCommon
{
public:
    CommandHandler ()
    {
        registerEvents();
    }
    /*
     * 入口函数，处理命令
     * @param params 处理后的命令
     * @return ResValueType
     */
    ResValueType handlerCommand (const CommandParams &commandParams)
    {
        Commands command = findCommand(commandParams.command);
        ResValueType res;
        if (command != Commands::END)
        {
            return BaseCommandHandler::handlerCommand(commandParams, command);
        }

        StructType structType = checkExpireKey(commandParams.key);
        switch (structType)
        {
            case StructType::NIL:
                break;
            case StructType::STRING:
                findStringCommand(commandParams, res);
                return res;
            case StructType::LIST:
                break;
            case StructType::HASH:
                break;
            case StructType::SET:
                break;
            case StructType::ZSET:
                break;
            case StructType::END:
                break;
        }

        bool foundIt = findStringCommand(commandParams, res);
        if (!foundIt)
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

    void checkExpireKeys ()
    {
        if (expireKey.empty())
            return;

        auto it = expireKey.begin();
        auto end = expireKey.end();
        ExpireMapType::iterator resIt;
        int i = 0;
        auto now = getNow();

        while (i++ < onceCheckExpireKeyMaxNum && (getNow() - now) < onceCheckExpireKeyMaxTime
            && it != end)
        {
            resIt = checkExpireKey(it);
            if (resIt != end)
                it = resIt;
            else
                ++it;
        }
    }
protected:
    void handlerFlushAll (const CommandParams &commandParams, ResValueType &resValue) override
    {
        if (!commandParams.key.empty() || !commandParams.params.empty())
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
            return;
        }
        clear();
        resValue.setOKFlag();
    }
    void handlerDel (const CommandParams &commandParams, ResValueType &resValue) override
    {
        size_t num = delKey(commandParams.key);
        resValue.setIntegerValue(static_cast<IntegerType>(num));
    }
    // 秒
    void handlerExpire (const CommandParams &commandParams, ResValueType &resValue) override
    {
        IntegerType expire;
        StructType structType;
        if (!expireCommandCheck(commandParams, resValue, expire, structType))
            return;

        setExpire(commandParams.key, structType, std::chrono::milliseconds(expire * 1000));
    }
    // 毫秒
    void handlerPExpire (const CommandParams &commandParams, ResValueType &resValue) override
    {
        IntegerType expire;
        StructType structType;
        if (!expireCommandCheck(commandParams, resValue, expire, structType))
            return;

        setExpire(commandParams.key, structType, std::chrono::milliseconds(expire));
    }
    // 秒
    void handlerTTL (const CommandParams &commandParams, ResValueType &resValue) override
    {
        if (!checkKeyIsValid(commandParams, resValue))
            return;

        auto it = expireKey.find(commandParams.key);
        if (it == expireKey.end())
            resValue.setIntegerValue(nilExpire);
        else
            resValue.setIntegerValue((it->second.second - getNow()).count() / 1000);
    }
    // 毫秒
    void handlerPTTL (const CommandParams &commandParams, ResValueType &resValue) override
    {
        if (!checkKeyIsValid(commandParams, resValue))
            return;

        auto it = expireKey.find(commandParams.key);
        if (it == expireKey.end())
            resValue.setIntegerValue(nilExpire);
        else
            resValue.setIntegerValue((it->second.second - getNow()).count());
    }

    void handlerKeys (const CommandParams &commandParams, ResValueType &resValue) override
    {
        // 目前只做keys *
        if (!checkKeyIsValid(commandParams, resValue))
            return;

        if (commandParams.key == "*")
            for (auto &v : keyOfStructType)
                resValue.setVectorValue(v.first);
    }
    void handlerFlushDb (const CommandParams &commandParams, ResValueType &resValue) override
    {
        // 目前不做多库
        handlerFlushAll(commandParams, resValue);
    }
    void handlerExists (const CommandParams &commandParams, ResValueType &resValue) override
    {
        if (!checkKeyIsValid(commandParams, resValue))
            return;

        KeyType key = commandParams.key;
        int i = 0, count = 0;
        AllKeyMapType::iterator it;
        auto end = keyOfStructType.end();
        do
        {
            it = keyOfStructType.find(key);
            if (it != end)
                count++;

            key = commandParams.params[i];
        } while (++i != commandParams.params.size());

        resValue.setIntegerValue(count);
    }
    void handlerType (const CommandParams &commandParams, ResValueType &resValue) override
    {
        if (!checkKeyIsValid(commandParams, resValue)
            || !checkHasParams(commandParams, resValue, 0))
            return;

        auto it = keyOfStructType.find(commandParams.key);
        if (it != keyOfStructType.end())
        {
            switch (it->second)
            {
                case StructType::STRING:
                    resValue.setStringValue("string");
                    return;
                case StructType::LIST:
                    resValue.setStringValue("list");
                    return;
                case StructType::HASH:
                    resValue.setStringValue("hash");
                    return;
                case StructType::SET:
                    resValue.setStringValue("set");
                    return;
                case StructType::ZSET:
                    resValue.setStringValue("zset");
                    return;
                case StructType::END:
                case StructType::NIL:
                    break;
            }
        }

        resValue.setStringValue("none");
    }
private:
    inline void clear () noexcept override
    {
        expireKey.clear();
        keyOfStructType.clear();
        stringCommandHandler.clear();
    }
    inline size_t delKey (const std::string &key) noexcept override
    {
        return delKeyEvent(key);
    }

    inline bool expireCommandCheck (
        const CommandParams &commandParams,
        ResValueType &resValue,
        IntegerType &expire,
        StructType &structType
    )
    {
        auto it = keyOfStructType.find(commandParams.key);
        if (it == keyOfStructType.end())
        {
            resValue.setIntegerValue(0);
            return false;
        }
        structType = it->second;
        return checkKeyIsValid(commandParams, resValue)
            && checkHasParams(commandParams, resValue, 1)
            && checkValueIsLongLong(commandParams, resValue, &expire);
    }

    void registerEvents ()
    {
        eventObserver.on(
            static_cast<int>(EventType::ADD_KEY),
            std::bind(&CommandHandler::addKeyEvent, this, std::placeholders::_1));
        eventObserver.on(
            static_cast<int>(EventType::RESET_EXPIRE),
            std::bind(&CommandHandler::resetExpire, this, std::placeholders::_1));
    }

    void addKeyEvent (void *arg)
    {
        auto eventAddObserverParams = static_cast<EventAddObserverParams *>(arg);
        keyOfStructType.emplace(eventAddObserverParams->key, eventAddObserverParams->structType);

        resetExpire(arg);
    }

    void resetExpire (void *arg)
    {
        auto eventAddObserverParams = static_cast<EventAddObserverParams *>(arg);

        setExpire(
            eventAddObserverParams->key,
            eventAddObserverParams->structType,
            eventAddObserverParams->expire
        );
    }

    void setExpire (
        const std::string &key,
        const StructType structType,
        const std::chrono::milliseconds expire
    )
    {
        if (expire > std::chrono::milliseconds(0))
        {
            auto it = expireKey.emplace(key, std::make_pair(structType, expire + getNow()));
            if (!it.second)
                it.first->second.second = expire + getNow();
        }
    }

    size_t delKeyEvent (const std::string &key)
    {
        auto it = expireKey.find(key);
        if (delKeyEvent(it) == expireKey.end())
            return 0;

        return 1;
    }

    ExpireMapType::iterator delKeyEvent (ExpireMapType::iterator &it)
    {
        size_t num = keyOfStructType.erase(it->first);
        ExpireMapType::iterator resIt;
        // 如果num为0则key不存在
        if (num)
        {
            const StructType structType = it->second.first;
            resIt = expireKey.erase(it);

            switch (structType)
            {
                case StructType::STRING:
                    stringCommandHandler.delKey(it->first);
                    break;
                case StructType::LIST:
                    break;
                case StructType::HASH:
                    break;
                case StructType::SET:
                    break;
                case StructType::ZSET:
                    break;
                case StructType::NIL:
                    break;
                case StructType::END:
                    break;
            }
        }

        return resIt;
    }

    StructType checkExpireKey (const std::string &key)
    {
        auto it = expireKey.find(key);
        if (checkExpireKey(it) != expireKey.end())
            return it->second.first;

        return StructType::NIL;
    }

    ExpireMapType::iterator checkExpireKey (ExpireMapType::iterator &it)
    {
        if (it != expireKey.end())
        {
            if (getNow() > it->second.second)
                return delKeyEvent(it);
        }

        return expireKey.end();
    }

    static inline std::chrono::milliseconds getNow () noexcept
    {
        return std::chrono::duration_cast <std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }

    inline bool findStringCommand (const CommandParams &commandParams, ResValueType &res)
    {
        StringCommandHandler::Commands
            stringCommand = StringCommandHandler::findCommand(commandParams.command);

        if (stringCommand != StringCommandHandler::Commands::END)
        {
            commandHandlerResByString(commandParams, res, stringCommand);
            return true;
        }

        return false;
    }

    inline void commandHandlerResByString (
        const CommandParams &commandParams,
        ResValueType &res,
        const StringCommandHandler::Commands &stringCommand
    )
    {
        if (!checkKeyType(commandParams, StructType::STRING, res))
            return;

        res = std::move(
            stringCommandHandler.handlerCommand(
                commandParams,
                stringCommand
            ));
    }

private:
    StringCommandHandler stringCommandHandler;

    static constexpr int onceCheckExpireKeyMaxNum = 20;
    static constexpr int nilExpire = -2;
    static constexpr std::chrono::milliseconds onceCheckExpireKeyMaxTime { 30 };
};

const char
    *BaseCommandHandler::commands[]
    { "flushall", "del", "expire", "pexpire", "ttl", "pttl", "keys", "flushdb", "exists", "type" };
constexpr std::chrono::milliseconds CommandHandler::onceCheckExpireKeyMaxTime;

#endif //LINUX_SERVER_LIB_KV_STORE_KV_COMMAND_H_
