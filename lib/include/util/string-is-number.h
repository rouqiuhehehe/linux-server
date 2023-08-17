//
// Created by 115282 on 2023/8/14.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_
#define LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_

#include <cstring>
namespace Utils
{
    namespace StringIsNumberHelper
    {
        inline bool stringIsInt (const std::string &str)
        {
            if (str.empty())
                return false;
            char *endPtr;
            std::strtol(str.c_str(), &endPtr, 10);
            return endPtr == (str.rbegin().operator->() + 1);
        }
    }
}
#endif //LINUX_SERVER_LIB_INCLUDE_STRING_IS_NUMBER_H_
