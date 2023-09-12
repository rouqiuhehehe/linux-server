//
// Created by 115282 on 2023/8/14.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_
#define LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_

#include <cstring>
#include <algorithm>
#include <numeric>
#include <cerrno>
#include <sstream>
#include <vector>

namespace Utils
{
    namespace StringHelper
    {
        inline void stringTolower (std::string &str)
        {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        }
        inline void stringToupper
            (std::string &str)
        {
            std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        }
        /*
         * @param str 输入字符串
         * @param split 分隔符
         * @param isContinuously 分割时是否判断连续的分隔符
         * @return std::vector<std::string>
         */
        inline std::vector <std::string> stringSplit (
            const std::string &str,
            const char split,
            bool isContinuously = true
        )
        {
            if (str.empty())
                return {};

            std::vector <std::string> res;
            size_t count = str.find(split);
            size_t index = 0;
            if (count == std::string::npos)
                count = str.size();
            while ((index + count) <= str.size())
            {
                res.emplace_back(str.substr(index, count));

                index += count + 1;
                count = 0;
                if (isContinuously)
                    while (index < str.size() && str[index] == split) index++;

                while ((index + count) < str.size() && str[index + ++count] != split);
            }

            return res;
        }

        inline bool stringIsLongLong (const std::string &str, long long *value = nullptr)
        {
            if (str.empty())
                return false;
            errno = 0;
            char *endPtr;
            auto res = std::strtoll(str.c_str(), &endPtr, 10);
            if (value)
                *value = res;

            long long minmax = str[0] == '-'
                               ? std::numeric_limits <long long>::min()
                               : std::numeric_limits <long long>::max();
            bool success =
                (endPtr == (str.rbegin().operator->() + 1)) && errno != ERANGE && res != minmax;
            errno = 0;

            return success;
        }

        inline bool stringIsDouble (const std::string &str, double *value = nullptr)
        {
            if (str.empty())
                return false;
            errno = 0;
            char *endPtr;
            auto res = std::strtod(str.c_str(), &endPtr);
            if (value)
                *value = res;

            double minmax = str[0] == '-'
                            ? -std::numeric_limits <double>::infinity()
                            : std::numeric_limits <double>::infinity();
            bool success =
                (endPtr == (str.rbegin().operator->() + 1)) && errno != ERANGE && res != minmax;

            errno = 0;
            return success;
        }

        template <class T>
        inline std::string toString (T val)
        {
            static std::ostringstream stream;
            stream.str("");
            stream << val;

            return stream.str();
        }
    }
}
#endif //LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_
