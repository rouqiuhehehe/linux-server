//
// Created by Yoshiki on 2023/6/2.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_UTIL_H_
#define LINUX_SERVER_LIB_INCLUDE_UTIL_H_

#ifdef __cplusplus
#include <cstdint>
namespace Utils
{
    template <class T>
    inline T *addressOf (const T &t)
    {
        return reinterpret_cast<T *>(&const_cast<char &>(reinterpret_cast<const volatile char &>(t)));
    }

    template <class T, uint32_t Size>
    inline uint32_t getArrLen (const T (&arr)[Size])
    {
        return Size;
    }

    class NonAbleCopy
    {
    public:
#if __cplusplus >= 201103L
        NonAbleCopy () = default;
        virtual ~NonAbleCopy () noexcept = default;
        NonAbleCopy (const NonAbleCopy &) = delete;
        NonAbleCopy &operator= (const NonAbleCopy &) = delete;
#else
        NonAbleCopy () {}
    virtual ~NonAbleCopy () {}
private:
    NonAbleCopy (const NonAbleCopy &) = default;
    NonAbleCopy &operator= (const NonAbleCopy &) = default;
#endif
    };

#if __cplusplus >= 201103L
    class NonAbleMoveCopy
    {
    public:
        NonAbleMoveCopy () = default;
        virtual ~NonAbleMoveCopy () noexcept = default;
    };

    class NonAbleAllCopy : NonAbleMoveCopy, NonAbleCopy
    {
    public:
        NonAbleAllCopy () = default;
        ~NonAbleAllCopy () noexcept override = default;
    };
#endif
}
#endif

#endif //LINUX_SERVER_LIB_INCLUDE_UTIL_H_
