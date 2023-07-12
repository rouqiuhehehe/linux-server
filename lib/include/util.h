//
// Created by Yoshiki on 2023/6/2.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_UTIL_H_
#define LINUX_SERVER_LIB_INCLUDE_UTIL_H_

#ifdef __cplusplus
#include <cstdint>

#include <locale>
#include <codecvt>
#include <random>
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

    static std::wstring_convert <std::codecvt_utf8 <wchar_t>, wchar_t> utf8_converter;

    inline int getRandomNum (int a, int b)
    {
        std::random_device device;
        std::default_random_engine engine(device());

        return std::uniform_int_distribution <int>(a, b)(engine);
    }

    inline std::string getRandomChinese ()
    {
        int seek = getRandomNum(1 << 13, 1 << 15);
        // 随机生成一个中文字符的 Unicode 值
        int unicode = 0x4e00 + seek % (0x9fa5 - 0x4e00 + 1);

        wchar_t ch = unicode;
        std::string uft8Str = utf8_converter.to_bytes(ch);

        return uft8Str;
    }
#endif
}
#endif

#endif //LINUX_SERVER_LIB_INCLUDE_UTIL_H_
