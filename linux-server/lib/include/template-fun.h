//
// Created by Administrator on 2023/4/24.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_TEMPLATE_FUN_H_
#define LINUX_SERVER_LIB_INCLUDE_TEMPLATE_FUN_H_

#include <type_traits>
template <class U, class ...Arg>
struct CheckStr
{
private:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
    template <class T, class ...arg>
    constexpr static auto check (int) -> decltype(std::declval <T>().str(std::declval <arg>()...), std::true_type {});
    template <class, class ...>
    constexpr static std::false_type check (...);
#pragma clang diagnostic pop

public:
    static constexpr bool ret = std::is_same <decltype(check <U, Arg...>(0)), std::true_type>::value;
};

#endif //LINUX_SERVER_LIB_INCLUDE_TEMPLATE_FUN_H_
