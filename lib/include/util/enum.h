//
// Created by 115282 on 2023/8/25.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_UTIL_ENUM_H_
#define LINUX_SERVER_LIB_INCLUDE_UTIL_ENUM_H_

#include "util.h"
#include <cstring>

template <class T>
class Enum : Utils::NonAbleAllCopy
{
public:
    using KeyType = int;

    static_assert(static_cast<KeyType>(T::NIL) == -1, "");
    static_assert(static_cast<KeyType>(T::END), "");

    template <size_t Size>
    explicit Enum (const char *const (&names)[Size])
        : names(reinterpret_cast<const char *const *>(names)), size(Size) {}

    inline bool assign (const char *name) const noexcept
    {
        return findNameIndex(name) == enumToInt(T::END);
    }

    inline const char *name (T enumVal) const noexcept
    {
        return names[enumToInt(enumVal)];
    }

    inline T enumVal (const char *name) const noexcept
    {
        return T(findNameIndex(name));
    }

    static inline int enumToInt (T enumVal)
    {
        return static_cast<int>(enumVal);
    }

private:
    inline size_t findNameIndex (const char *name) const noexcept
    {
        for (size_t i = enumToInt(T::NIL) + 1; i < enumToInt(T::END); ++i)
        {
            if (strcasecmp(name, names[i]) == 0)
                return i;
        }
    }

    const size_t size;
    const char *const *names;
    // static constexpr const char **names = Names;

};
#endif //LINUX_SERVER_LIB_INCLUDE_UTIL_ENUM_H_
