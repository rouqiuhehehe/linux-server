//
// Created by Yoshiki on 2023/8/13.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
#include <string>
#include <chrono>

#define NIL_MESSAGE "(nil)"
#define OK_MESSAGE "OK"
#define ERROR_MESSAGE_HELPER(str) "(error) " str

using KeyType = std::string;
typedef struct
{
    std::string command;
    std::string key;
    std::vector <std::string> params;
} CommandParams;
struct ValueType
{
    enum SetModel
    {
        DEFAULT,
        NX,
        XX
    };
    std::string value;
    explicit ValueType (std::string &&value)
        : value(std::move(value)) {}
    virtual bool isNil () const noexcept
    {
        return false;
    }
    virtual bool isOk () const noexcept
    {
        return false;
    }

protected:
    ValueType () = default;
};
struct ValidValueType : public ValueType
{
    std::chrono::milliseconds expireTime { -1 };
};
struct StringValueType : public ValidValueType
{
    //NX ：只在键不存在时，才对键进行设置操作。 SET key value NX 效果等同于 SETNX key value 。
    // XX ：只在键已经存在时，才对键进行设置操作。
    SetModel setModel = DEFAULT;
};
struct NilType : public ValueType
{
    NilType ()
        : ValueType(NIL_MESSAGE) {}

    bool isNil () const noexcept override
    {
        return true;
    }
};

struct OkType : public ValueType
{
    OkType ()
        : ValueType(OK_MESSAGE) {}

    bool isOk () const noexcept override
    {
        return true;
    }
};
struct Error : public ValueType
{
    enum ErrorType
    {
        WRONGTYPE,
        UNKNOWN_COMMAND,
        WRONG_NUMBER
    };

    explicit Error (ErrorType type, const CommandParams &commandParams)
    {
        char msg[128];
        switch (type)
        {
            case WRONGTYPE:
                value = ERROR_MESSAGE_HELPER(
                            "WRONGTYPE Operation against a key holding the wrong kind of value");
                break;
            case UNKNOWN_COMMAND:
                sprintf(
                    msg,
                    ERROR_MESSAGE_HELPER("ERR unknown command '%s', with args beginning with:"),
                    commandParams.key.c_str());
                value = msg;
                break;
            case WRONG_NUMBER:
                sprintf(
                    msg,
                    ERROR_MESSAGE_HELPER("ERR wrong number of arguments for '%s' command"),
                    commandParams.key.c_str());
                value = msg;
                break;
        }
    }
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_VALUE_H_
