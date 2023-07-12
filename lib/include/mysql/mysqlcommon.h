//
// Created by 115282 on 2023/7/5.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLCOMMON_H_
#define HELLO_WORLD_INCLUDE_MYSQLCOMMON_H_
#include "printf-color.h"

using INT24 = struct
{
    union
    {
        int int24: 24;
        uint uint24: 24;
    };
} __attribute__((packed));

#define PRINT_MYSQL_ERROR(db, str, ...) PRINT_ERROR(str ": %s", ##__VA_ARGS__, mysql_error(db))
#define PRINT_MYSQL_STMT_ERROR(stmt, str, ...) PRINT_ERROR(str ": %s", ##__VA_ARGS__, mysql_stmt_error(stmt))
#endif //HELLO_WORLD_INCLUDE_MYSQLCOMMON_H_
