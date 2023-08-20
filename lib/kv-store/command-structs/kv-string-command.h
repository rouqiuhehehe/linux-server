//
// Created by 115282 on 2023/8/15.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_STRING_COMMAND_H_
#define LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_STRING_COMMAND_H_
#include "command-common.h"

class StringCommandHandler : public CommandCommon
{
public:
    enum Commands
    {
        NIL = -1,
        SET,
        GET,
        INCR,
        INCRBY,
        INCRBYFLOAT,
        DECR,
        DECRBY,
        APPEND,
        GETSET,
        END
    };
    FIND_COMMAND

    inline ResValueType handlerCommand (
        const CommandParams &commandParams,
        Commands cmd,
        EventsObserverType &eventObserver
    )
    {
        ResValueType resValue;
        // 检查是否有key
        bool success = checkKeyIsValid(commandParams, resValue);
        if (!success)
            return resValue;

        switch (cmd)
        {
            case Commands::SET:
                handlerSet(commandParams, resValue, eventObserver);
                break;
            case Commands::GET:
                handlerGet(commandParams, resValue);
                break;
            case Commands::INCR:
                handlerIncr(commandParams, resValue);
                break;
            case Commands::INCRBY:
                handlerIncrBy(commandParams, resValue);
                break;
            case Commands::DECR:
                handlerDecr(commandParams, resValue);
                break;
            case Commands::DECRBY:
                handlerDecrBy(commandParams, resValue);
                break;
            case Commands::APPEND:
                handlerAppend(commandParams, resValue);
                break;
            case Commands::GETSET:
                handlerGetset(commandParams, resValue);
                break;
            case INCRBYFLOAT:
                handlerIncrByFloat(commandParams, resValue);
                break;
            case Commands::NIL:
            case Commands::END:
                break;
        }

        return resValue;
    }

    inline void clear () noexcept override {}
    inline size_t delKey (const std::string &key) noexcept override
    {
        return keyValues.erase(key);
    }
private:
    void handlerSet (
        const CommandParams &commandParams,
        ResValueType &resValue,
        EventsObserverType &eventObserver
    )
    {
        // 检查参数长度 是否缺少参数
        bool success = checkHasParams(commandParams, resValue, -1);
        if (!success)
            return;

        StringValueType value;

        // 初始化emit参数
        static EventAddObserverParams eventAddObserverParams;
        eventAddObserverParams.structType = StructType::STRING;
        eventAddObserverParams.key = commandParams.key;

        // 检查拓展参数 NX|XX EX|PX GET
        success = handlerExtraParams(commandParams, value, resValue, eventAddObserverParams);
        if (!success)
            return;

        // 填充value
        value.value = commandParams.params[0];
        resValue.setOKFlag();
        auto it = keyValues.find(commandParams.key);
        if (it == keyValues.end())
        {
            if (value.setModel == StringValueType::SetModel::NX)
            {
                resValue.setNilFlag();
                return;
            }
            if (value.isReturnOldValue)
                resValue.setNilFlag();

            keyValues.emplace(commandParams.key, value);

            eventObserver.emit(static_cast<int>(EventType::ADD_KEY), &eventAddObserverParams);
            return;
        }

        if (value.setModel == StringValueType::SetModel::XX)
        {
            resValue.setNilFlag();
            return;
        }
        if (value.isReturnOldValue)
            resValue = std::move(it->second);

        if (eventAddObserverParams.expire != std::chrono::milliseconds(0))
            eventObserver.emit(static_cast<int>(EventType::RESET_EXPIRE), &eventAddObserverParams);

        it->second = value;
    }

    void handlerGet (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 0);
        if (!success)
            return;

        auto it = keyValues.find(commandParams.key);
        if (it == keyValues.end())
            resValue.setNilFlag();
        else
            resValue = it->second;
    }

    void handlerIncr (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 0);
        if (!success)
            return;

        handlerIncrCommon(commandParams, resValue, 1);
    }

    void handlerIncrBy (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 1);
        if (!success)
            return;

        IntegerType integer;
        if (!checkValueIsLongLong(commandParams, resValue, &integer))
            return;

        handlerIncrCommon(commandParams, resValue, integer);
    }

    void handlerDecr (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 0);
        if (!success)
            return;

        handlerIncrCommon(commandParams, resValue, -1);
    }

    void handlerDecrBy (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 1);
        if (!success)
            return;

        IntegerType integer;
        if (!checkValueIsLongLong(commandParams, resValue, &integer))
            return;

        handlerIncrCommon(commandParams, resValue, -integer);
    }
    void handlerAppend (const CommandParams &commandParams, ResValueType &resValue) {}
    void handlerGetset (const CommandParams &commandParams, ResValueType &resValue) {}

    void handlerIncrByFloat (const CommandParams &commandParams, ResValueType &resValue)
    {
        bool success = checkHasParams(commandParams, resValue, 1);
        if (!success)
            return;

        FloatType doubleValue;
        if (!Utils::StringHelper::stringIsDouble(commandParams.params[0], &doubleValue))
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::VALUE_NOT_FLOAT);
            return;
        }

        auto it = keyValues.find(commandParams.key);
        if (it == keyValues.end())
        {
            ValueType value;
            value.value = doubleValue;
            keyValues.emplace(commandParams.key, value);

            resValue = value;
            resValue.model = ResValueType::ReplyModel::REPLY_STRING;
        }
        else
        {
            FloatType oldValue;
            if (!Utils::StringHelper::stringIsDouble(it->second.value, &oldValue))
                resValue.setErrorStr(commandParams, ResValueType::ErrorType::VALUE_NOT_FLOAT);
            else
            {
                if (Utils::MathHelper::doubleCalculateWhetherOverflow <std::plus <FloatType>>(
                    oldValue,
                    doubleValue
                ))
                {
                    resValue.setErrorStr(
                        commandParams,
                        ResValueType::ErrorType::INCR_OR_DECR_OVERFLOW
                    );
                    return;
                }

                it->second.value = std::to_string(doubleValue + oldValue);
                resValue = it->second;
                resValue.model = ResValueType::ReplyModel::REPLY_STRING;
            }
        }
    }

private:
    static bool handlerExtraParams (
        const CommandParams &commandParams,
        StringValueType &value,
        ResValueType &resValue,
        EventAddObserverParams &eventAddObserverParams
    )
    {
        // 有额外选项 nx|xx get ex|px
        // set key 123 nx get ex 1000
        // 第一个值为value 不处理
        if (commandParams.params.size() > 1)
        {
            std::string command;
            IntegerType integer;
            auto it = commandParams.params.begin() + 1;
            for (; it < commandParams.params.end(); ++it)
            {
                command = *it;
                Utils::StringHelper::stringTolower(command);
                if (command == EXTRA_PARAMS_NX)
                {
                    if (value.setModel != StringValueType::SetModel::XX)
                        value.setModel = StringValueType::SetModel::NX;
                    else
                    {
                        resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
                        return false;
                    }
                }
                else if (command == EXTRA_PARAMS_XX)
                {
                    if (value.setModel != StringValueType::SetModel::NX)
                        value.setModel = StringValueType::SetModel::XX;
                    else
                    {
                        resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
                        return false;
                    }
                }
                else if (command == EXTRA_PARAMS_GET)
                    value.isReturnOldValue = true;
                else if (command == EXTRA_PARAMS_PX)
                {
                    if (value.timeModel != StringValueType::TimeModel::EX)
                    {
                        it++;
                        if (!checkValueIsLongLong(commandParams, *it, resValue, &integer))
                            return false;
                        if (!checkExpireIsValid(commandParams, integer, resValue))
                            return false;
                        value.timeModel = StringValueType::TimeModel::PX;
                        eventAddObserverParams.expire = std::chrono::milliseconds { integer };
                    }
                    else
                    {
                        resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
                        return false;
                    }
                }
                else if (command == EXTRA_PARAMS_EX)
                {
                    if (value.timeModel != StringValueType::TimeModel::PX)
                    {
                        it++;
                        if (!checkValueIsLongLong(commandParams, *it, resValue, &integer))
                            return false;
                        if (!checkExpireIsValid(commandParams, integer, resValue))
                            return false;
                        value.timeModel = StringValueType::TimeModel::EX;
                        eventAddObserverParams.expire =
                            std::chrono::milliseconds { integer * 1000 };
                    }
                    else
                    {
                        resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
                        return false;
                    }
                }
                else
                {
                    resValue.setErrorStr(commandParams, ResValueType::ErrorType::SYNTAX_ERROR);
                    return false;
                }
            }
        }

        return true;
    }

    void handlerIncrCommon (
        const CommandParams &commandParams,
        ResValueType &resValue,
        const IntegerType step
    )
    {
        auto it = keyValues.find(commandParams.key);
        if (it == keyValues.end())
        {
            ValueType value;
            value.value = step;
            keyValues.emplace(commandParams.key, value);

            resValue = value;
            resValue.model = ResValueType::ReplyModel::REPLY_INTEGER;
        }
        else
        {
            IntegerType integer;
            if (!Utils::StringHelper::stringIsLongLong(it->second.value, &integer))
                resValue.setErrorStr(commandParams, ResValueType::ErrorType::VALUE_NOT_INTEGER);
            else
            {
                if (Utils::MathHelper::integerPlusOverflow(integer, step))
                {
                    resValue.setErrorStr(
                        commandParams,
                        ResValueType::ErrorType::INCR_OR_DECR_OVERFLOW
                    );
                    return;
                }
                it->second.value = std::to_string(step + integer);
                resValue = it->second;
                resValue.model = ResValueType::ReplyModel::REPLY_INTEGER;
            }
        }
    }

    std::unordered_map <KeyType, ValueType> keyValues {};
    static const char *commands[];
};

const char *StringCommandHandler::commands[]
    { "set", "get", "incr", "incrby", "decr", "decrby", "append", "getset" };
#endif //LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_KV_STRING_COMMAND_H_
