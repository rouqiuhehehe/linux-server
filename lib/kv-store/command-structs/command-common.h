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
protected:
    /*
     * 检查参数
     * @param commandParams
     * @param resValue
     * @param paramsNum 检查参数的个数，0为不需要参数，-1 为不检查参数个数，可以有任意长度参数
     * @return
     */
    static bool checkHasParams (
        const CommandParams &commandParams,
        ResValueType &resValue,
        int paramsNum
    )
    {
        bool hasError = paramsNum == -1 ? commandParams.params.empty() : (commandParams.params.size() != paramsNum);
        if (hasError)
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::WRONG_NUMBER);
            return false;
        }

        return true;
    }

    static bool checkKeyIsValid (const CommandParams &commandParams, ResValueType &resValue)
    {
        if (commandParams.key.empty())
        {
            resValue.setErrorStr(commandParams, ResValueType::ErrorType::WRONG_NUMBER);
            return false;
        }

        return true;
    }
};
#endif //LINUX_SERVER_LIB_KV_STORE_COMMAND_STRUCTS_COMMAND_COMMON_H_
