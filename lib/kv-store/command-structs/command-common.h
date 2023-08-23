//
// Created by 115282 on 2023/8/15.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_COMMAND_COMMON_H_
#define LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_COMMAND_COMMON_H_
#define FIND_COMMAND                                                        \
        static inline Commands findCommand (const std::string &command)     \
        {                                                                   \
            int cmd = static_cast<int>(Commands::NIL) + 1;                  \
            for (; cmd != static_cast<int>(Commands::END); ++cmd)           \
            {                                                               \
                if (std::strcmp(command.c_str(), commands[cmd]) == 0)       \
                break;                                                      \
            }                                                               \
                                                                            \
            return Commands(cmd);                                           \
        }

class CommandCommon
{
public:
    inline virtual void clear () noexcept = 0;
    inline virtual size_t delKey (const std::string &) noexcept = 0;

protected:
    /*
     * 检查参数
     * @param commandParams
     * @param resValue
     * @param paramsNum 检查参数的个数，0为不需要参数，-1 为不检查参数个数，可以有任意长度参数
     * @return
     */
    static inline bool checkHasParams (
        const CommandParams &commandParams,
        ResValueType &resValue,
        int paramsNum
    )
    {
        bool hasError =
            paramsNum == -1 ? commandParams.params.empty() : (commandParams.params.size()
                != paramsNum);
        if (hasError)
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::WRONG_NUMBER);
            return false;
        }

        return true;
    }

    // 检查是否存在key
    static inline bool checkKeyIsValid (const CommandParams &commandParams, ResValueType &resValue)
    {
        if (commandParams.key.empty())
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::WRONG_NUMBER);
            return false;
        }

        return true;
    }

    // 检查value[0]是否是数字
    static inline bool checkValueIsLongLong (
        const CommandParams &commandParams,
        ResValueType &resValue,
        IntegerType *integer = nullptr
    )
    {
        if (!Utils::StringHelper::stringIsLongLong(commandParams.params[0], integer))
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::VALUE_NOT_INTEGER);
            return false;
        }

        return true;
    }

    static inline bool checkValueIsLongLong (
        const CommandParams &commandParams,
        const std::string &checkValue,
        ResValueType &resValue,
        IntegerType *integer = nullptr
    )
    {
        if (!Utils::StringHelper::stringIsLongLong(checkValue, integer))
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::VALUE_NOT_INTEGER);
            return false;
        }

        return true;
    }

    // 检查过期时间是否是正整数
    static inline bool checkExpireIsValid (
        const CommandParams &commandParams,
        const IntegerType expire,
        ResValueType &resValue
    )
    {
        if (expire <= 0)
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::INVALID_EXPIRE_TIME);
            return false;
        }

        return true;
    }

    static EventAddObserverParams eventAddObserverParams;
    static EventsObserverType eventObserver;
};

EventAddObserverParams CommandCommon::eventAddObserverParams;
EventsObserverType CommandCommon::eventObserver;
#endif //LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_COMMAND_COMMON_H_
