//
// Created by Yoshiki on 2023/6/2.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_UTIL_H_
#define LINUX_SERVER_LIB_INCLUDE_UTIL_H_

#define UUID_FILE_PATH "/proc/sys/kernel/random/uuid"
#define UUID_STR_LEN 37

#ifdef __cplusplus
#include <cstdint>

#include <locale>
#include <codecvt>
#include <random>
#include <fstream>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

namespace Utils
{
    inline std::string getIpAndHost (const struct sockaddr_in &sockaddrIn)
    {
        char *ip = inet_ntoa(sockaddrIn.sin_addr);
        uint16_t port = ntohs(sockaddrIn.sin_port);

        return std::string(ip) + ":" + std::to_string(port);
    }
    inline std::string getUuid ()
    {
        std::ifstream ifStream(UUID_FILE_PATH);
        if (ifStream.is_open())
        {
            char uuid[UUID_STR_LEN];
            ifStream.get(uuid, UUID_STR_LEN);

            if (ifStream.gcount() != UUID_STR_LEN - 1)
            {
                std::cerr << "create uuid length error, need 36, real " << ifStream.gcount()
                          << std::endl;
                ifStream.close();
                return {};
            }

            ifStream.close();
            return uuid;
        }
        else
        {
            std::cerr << "can't open the file " UUID_FILE_PATH ", error str : "
                      << strerror(errno) << std::endl;
            return {};
        }
    }

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
        NonAbleMoveCopy (NonAbleMoveCopy &&) noexcept = delete;
        NonAbleMoveCopy &operator= (NonAbleMoveCopy &&) noexcept = delete;
    };

    class NonAbleAllCopy : NonAbleMoveCopy, NonAbleCopy
    {
    public:
        NonAbleAllCopy () = default;
        ~NonAbleAllCopy () noexcept override = default;
    };

    inline int getRandomNum (int a, int b)
    {
        static std::random_device device;
        static std::default_random_engine engine(device());

        return std::uniform_int_distribution <int>(a, b)(engine);
    }

    inline std::string getRandomChinese ()
    {
        int seek = getRandomNum(0, 0x9fa5 - 0x4e00 + 1);
        // 随机生成一个中文字符的 Unicode 值
        int unicode = 0x4e00 + seek % (0x9fa5 - 0x4e00 + 1);

        static std::wstring_convert <std::codecvt_utf8 <wchar_t>, wchar_t> utf8_converter;
        std::string uft8Str = utf8_converter.to_bytes(unicode);

        return uft8Str;
    }
#endif
}

#else
#include <string.h>
#include <stdio.h>
#include <errno.h>

inline static void getUuid (char *src)
{
    FILE *file = fopen(UUID_FILE_PATH, "r");
    if (!file)
    {
        fprintf(stderr, "can't open the file " UUID_FILE_PATH " error str : %s\n", strerror(errno));
        return;
    }

    fgets(src, UUID_STR_LEN, file);
    fclose(file);
}
#endif

#endif //LINUX_SERVER_LIB_INCLUDE_UTIL_H_
