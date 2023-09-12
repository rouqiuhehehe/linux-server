//
// Created by 115282 on 2023/9/12.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
#ifndef LINUX_SERVER_SRC_TEST_CPP_20_TEST_REFLECT_HELPER_H_
#define LINUX_SERVER_SRC_TEST_CPP_20_TEST_REFLECT_HELPER_H_

#include <type_traits>
struct AnyType
{
    template <class T>
    operator T (); // NOLINT
};
#if __cplusplus >= 202002L
template <class T>
consteval std::size_t CountMember (auto &&...Args)
{
    if constexpr(!
        requires { T { Args... }; })
        return sizeof...(Args) - 1;
    else
        // 每次添加一个万能类型进去
        return CountMember <T>(Args..., AnyType {});
}
#elif __cplusplus >= 201103L
#if __cplusplus < 201703L
template <typename...>
using void_t = void;
#else
using std::void_t;
#endif

template <class T, class  = void, class ...Args>
struct CountMember
{
    constexpr static std::size_t value = sizeof...(Args) - 1;
};

template <class T, class ...Args>
struct CountMember <T, void_t <decltype(T { Args {}... })>, Args...>
{
    constexpr static std::size_t value = CountMember <T, void, Args..., AnyType>::value;
};
#endif
#endif //LINUX_SERVER_SRC_TEST_CPP_20_TEST_REFLECT_HELPER_H_

#pragma clang diagnostic pop