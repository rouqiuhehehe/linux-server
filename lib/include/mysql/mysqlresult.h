//
// Created by 115282 on 2023/6/20.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLRESULT_H_
#define HELLO_WORLD_INCLUDE_MYSQLRESULT_H_

#include "global.h"
#include <mysql.h>
#include "printf-color.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include "mysql/mysqlcommon.h"
#include "global.h"

class MysqlResSet;
class MysqlResSetPrivate : public BasePrivate
{
    DECLARE_PUBLIC_Q(MysqlResSet);
public:
    MysqlResSetPrivate (const MysqlResSetPrivate &) = default;
    MysqlResSetPrivate &operator= (const MysqlResSetPrivate &) = default;

    ~MysqlResSetPrivate () noexcept override = default;

    friend std::ostream &operator<< (std::ostream &os, const MysqlResSetPrivate *rhs);
protected:
    MysqlResSetPrivate (MYSQL_RES *, MysqlResSet *);
    MysqlResSetPrivate (MYSQL_STMT *, MysqlResSet *);

    using ResMapType = std::unordered_map <std::string, std::string>;

    void insertResMap (MYSQL_ROW);
    static void setResultBind (MYSQL_BIND *, MYSQL_FIELD *);

    std::vector <std::string> fields;
    std::vector <ResMapType> results;
    bool isValid = false;
};
class MysqlResSet : public Base
{
    DECLARE_PRIVATE_D(MysqlResSet);
public:
    CLASS_IS_VALID(MysqlResSet, d->isValid)
    CLASS_DEFAULT_ALL_COPY_CONSTRUCTOR(MysqlResSet, Base)

    explicit MysqlResSet (MYSQL_RES *);
    explicit MysqlResSet (MYSQL_STMT *);
    MysqlResSet () = default;
    ~MysqlResSet () noexcept override = default;

    inline static std::string mysqlTimeFormatter (MYSQL_TIME *);
    inline static std::string mysqlBindTypeToString (MYSQL_BIND *);

    inline size_t size () const
    {
        const D_PTR(MysqlResSet);
        return d->results.size();
    }

    inline const std::vector <MysqlResSetPrivate::ResMapType> &getResults () const noexcept
    {
        const D_PTR(MysqlResSet);
        return d->results;
    }

    inline friend std::ostream &operator<< (std::ostream &os, const MysqlResSet &rhs)
    {
        const MysqlResSetPrivate *const d = rhs.d_fun();
        os << d;
        return os;
    }
protected:
    explicit MysqlResSet (MysqlResSetPrivate *);
};

using MysqlResSetSharedPtr_t = std::shared_ptr <MysqlResSet>;
std::string MysqlResSet::mysqlTimeFormatter (MYSQL_TIME *mysqlTime)
{
    std::stringstream stream;
    stream << std::to_string(mysqlTime->year) << "-" << std::to_string(mysqlTime->month) << "-"
           << std::to_string(mysqlTime->day) << " "
           << (mysqlTime->hour < 10
               ? "0" + std::to_string(mysqlTime->hour) : std::to_string(mysqlTime->hour))
           << ":" << std::to_string(mysqlTime->minute) << ":" << std::to_string(mysqlTime->second);

    return stream.str();
}

std::string MysqlResSet::mysqlBindTypeToString (MYSQL_BIND *bind)
{
    switch (bind->buffer_type)
    {
        case MYSQL_TYPE_TINY:
            return std::to_string(*static_cast<char *>(bind->buffer));
        case MYSQL_TYPE_INT24:
            return std::to_string(static_cast<INT24 *>(bind->buffer)->int24);
        case MYSQL_TYPE_LONG:
            return std::to_string(*static_cast<int32_t *>(bind->buffer));
        case MYSQL_TYPE_LONGLONG:
            return std::to_string(*static_cast<int64_t *>(bind->buffer));
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            return { static_cast<char *>(bind->buffer), bind->buffer_length };
        case MYSQL_TYPE_TIMESTAMP:
            return MysqlResSet::mysqlTimeFormatter((MYSQL_TIME *)bind->buffer);
        case MYSQL_TYPE_DECIMAL:
            break;
        case MYSQL_TYPE_SHORT:
            break;
        case MYSQL_TYPE_FLOAT:
            break;
        case MYSQL_TYPE_DOUBLE:
            break;
        case MYSQL_TYPE_NULL:
            break;
        case MYSQL_TYPE_DATE:
            break;
        case MYSQL_TYPE_TIME:
            break;
        case MYSQL_TYPE_DATETIME:
            break;
        case MYSQL_TYPE_YEAR:
            break;
        case MYSQL_TYPE_NEWDATE:
            break;
        case MYSQL_TYPE_VARCHAR:
            break;
        case MYSQL_TYPE_BIT:
            break;
        case MYSQL_TYPE_TIMESTAMP2:
            break;
        case MYSQL_TYPE_DATETIME2:
            break;
        case MYSQL_TYPE_TIME2:
            break;
        case MYSQL_TYPE_JSON:
            break;
        case MYSQL_TYPE_NEWDECIMAL:
            break;
        case MYSQL_TYPE_ENUM:
            break;
        case MYSQL_TYPE_SET:
            break;
        case MYSQL_TYPE_TINY_BLOB:
            break;
        case MYSQL_TYPE_MEDIUM_BLOB:
            break;
        case MYSQL_TYPE_LONG_BLOB:
            break;
        case MYSQL_TYPE_BLOB:
            break;
        case MYSQL_TYPE_GEOMETRY:
            break;
    }
    return {};
}
#endif //HELLO_WORLD_INCLUDE_MYSQLRESULT_H_
