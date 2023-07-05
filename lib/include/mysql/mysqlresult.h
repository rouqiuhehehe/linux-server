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
#include "global.h"

#define PRINT_MYSQL_ERROR(db, str, ...) PRINT_ERROR(str ": %s\n", ##__VA_ARGS__, mysql_error(db))

class MysqlResSet;
class MysqlResSetPrivate : public BasePrivate
{
    DECLARE_PUBLIC_Q(MysqlResSet);
public:
    explicit MysqlResSetPrivate (MYSQL_RES *, MysqlResSet *);

    MysqlResSetPrivate (const MysqlResSetPrivate &) = default;
    MysqlResSetPrivate &operator= (const MysqlResSetPrivate &) = default;

    ~MysqlResSetPrivate () noexcept override = default;

    friend std::ostream &operator<< (std::ostream &os, const MysqlResSetPrivate *rhs);
protected:
    using ResMapType = std::unordered_map <std::string, std::string>;

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
    MysqlResSet () = default;
    ~MysqlResSet () noexcept override = default;

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

#endif //HELLO_WORLD_INCLUDE_MYSQLRESULT_H_
