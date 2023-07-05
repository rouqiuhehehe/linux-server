//
// Created by 115282 on 2023/6/20.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_
#define HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_

#include "mysqlresult.h"

class MysqlConnect;
class MysqlConnectPrivate : public BasePrivate
{
    DECLARE_PUBLIC_Q(MysqlConnect);
public:
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnectPrivate)

    MysqlConnectPrivate (
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname,
        MysqlConnect *parent
    );
    ~MysqlConnectPrivate () noexcept override;

protected:
    MYSQL *db;
    bool hasError = false;

private:
    static std::string transactionSQL;
    static std::string commitSQL;
    static std::string rollbackSQL;
};

class MysqlConnect : public Base
{
    DECLARE_PRIVATE_D(MysqlConnect);
public:
    CLASS_IS_VALID(MysqlConnect, !d->hasError)
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnect)
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(MysqlConnect, Base)

    MysqlConnect (
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname
    );
    ~MysqlConnect () noexcept override = default;

    bool exec (const char *sql);
    MysqlResSet execQuery (const char *sql);

    bool transaction ();
    bool rollback ();
    bool commit ();

    inline MYSQL *getMysql () noexcept
    {
        D_PTR(MysqlConnect);
        return d->db;
    }

    inline const MYSQL *getMysql () const noexcept
    {
        const D_PTR(MysqlConnect);
        return d->db;
    }

    inline uint64_t getAffectedRows () const noexcept
    {
        const D_PTR(MysqlConnect);
        return mysql_affected_rows(d->db);
    }

    inline bool hasError () const noexcept
    {
        const D_PTR(MysqlConnect);
        return d->hasError;
    }

protected:
    explicit MysqlConnect (MysqlConnectPrivate *);
};

#endif //HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_
