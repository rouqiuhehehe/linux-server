//
// Created by Yoshiki on 2023/8/13.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
#include <string>
#include <chrono>
#include <sstream>

#define EXTRA_PARAMS_NX "nx"
#define EXTRA_PARAMS_XX "xx"
#define EXTRA_PARAMS_EX "ex"
#define EXTRA_PARAMS_PX "px"
#define EXTRA_PARAMS_GET "get"

#define MESSAGE_NIL "(nil)"
#define MESSAGE_OK "OK"
#define ERROR_MESSAGE_HELPER(str) "(error) " str

using KeyType = std::string;
using IntegerType = long long;
using FloatType = double;
typedef struct
{
    std::string command;
    std::string key;
    std::vector <std::string> params;
} CommandParams;
struct ValueType
{
    std::string value;
    std::chrono::milliseconds expireTime { -1 };
};
struct StringValueType : public ValueType
{
    enum class SetModel
    {
        DEFAULT,
        NX,
        XX
    };
    enum class TimeModel
    {
        NIL,
        // seconds
        EX,
        // milliseconds
        PX
    };
    //NX ：只在键不存在时，才对键进行设置操作。 SET key value NX 效果等同于 SETNX key value 。
    // XX ：只在键已经存在时，才对键进行设置操作。
    SetModel setModel = SetModel::DEFAULT;
    TimeModel timeModel = TimeModel::NIL;

    // 是否返回之前储存的值 get 可选参数
    bool isReturnOldValue = false;
};

struct ResValueType : public ValueType
{
    enum class ReplyModel
    {
        REPLY_UNKNOWN,
        REPLY_STATUS,
        REPLY_ERROR,
        REPLY_INTEGER,
        REPLY_NIL,
        REPLY_STRING,
        REPLY_ARRAY
    };

    enum class ErrorType
    {
        // 设置一个类型不相同的key报错   比如 lset key  后  get key
        WRONGTYPE,
        // 未定义的 命令报错
        UNKNOWN_COMMAND,
        // set key 为number 时报错
        WRONG_NUMBER,
        // incr decr incrBy decrBy value不为数字时 报错
        VALUE_NOT_INTEGER,
        // incrByFloat params不为数字或value 不为数字时报错
        VALUE_NOT_FLOAT,
        // ex : set 同时设置NX XX 时会报错
        SYNTAX_ERROR,
        // incr或decr 后溢出报错
        INCR_OR_DECR_OVERFLOW
    };

    ResValueType () = default;
    explicit ResValueType (const ValueType &rhs) noexcept
    {
        operator=(rhs);
    }
    explicit ResValueType (ValueType &&rhs) noexcept
    {
        operator=(std::move(rhs));
    }
    ResValueType &operator= (const ValueType &rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        expireTime = rhs.expireTime;
        value = rhs.value;

        return *this;
    }
    ResValueType &operator= (ValueType &&rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        expireTime = rhs.expireTime;
        value = std::move(rhs.value);

        return *this;
    }

    void setErrorStr (const CommandParams &commandParams, ErrorType type)
    {
        static char msg[128];
        static std::stringstream paramsStream;
        model = ReplyModel::REPLY_ERROR;
        switch (type)
        {
            case ErrorType::WRONGTYPE:
                value = ERROR_MESSAGE_HELPER(
                            "WRONGTYPE Operation against a key holding the wrong kind of value");
                break;
            case ErrorType::UNKNOWN_COMMAND:
                paramsStream.str("");
                paramsStream << '\'' << commandParams.key << "\' ";
                for (auto &v : commandParams.params)
                    paramsStream << '\'' << v << "\' ";

                sprintf(
                    msg,
                    ERROR_MESSAGE_HELPER("ERR unknown command '%s', with args beginning with: %s%c"),
                    commandParams.command.c_str(), paramsStream.str().c_str(), '\0'
                );
                paramsStream.clear();
                value = msg;
                break;
            case ErrorType::WRONG_NUMBER:
                sprintf(
                    msg,
                    ERROR_MESSAGE_HELPER("ERR wrong number of arguments for '%s' command%c"),
                    commandParams.command.c_str(), '\0'
                );
                value = msg;
                break;
            case ErrorType::VALUE_NOT_INTEGER:
                value = ERROR_MESSAGE_HELPER("ERR value is not an integer or out of range");
                break;
            case ErrorType::VALUE_NOT_FLOAT:
                value = ERROR_MESSAGE_HELPER("ERR value is not a valid float");
                break;
            case ErrorType::SYNTAX_ERROR:
                value = ERROR_MESSAGE_HELPER("ERR syntax error");
                break;
            case ErrorType::INCR_OR_DECR_OVERFLOW:
                value = ERROR_MESSAGE_HELPER("ERR increment or decrement would overflow");
                break;
        }
    }

    inline void setNilFlag ()
    {
        model = ReplyModel::REPLY_NIL;
        value = MESSAGE_NIL;
    }
    inline void setOKFlag ()
    {
        model = ReplyModel::REPLY_STATUS;
        value = MESSAGE_OK;
    }

    ReplyModel model = ReplyModel::REPLY_UNKNOWN;
    std::vector <ResValueType> elements {};
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
