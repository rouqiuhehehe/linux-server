//
// Created by 115282 on 2023/6/20.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_
#define HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_

#include "mysqlresult.h"
#include "mysqlstmt.h"

class MysqlConnect;
class MysqlConnectPrivate : public BasePrivate
{
    DECLARE_PUBLIC_Q(MysqlConnect);
public:
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnectPrivate)

    ~MysqlConnectPrivate () noexcept override;

protected:
    MysqlConnectPrivate (
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname,
        bool debug,
        MysqlConnect *parent
    );

    MYSQL *db;
    bool isValid = false;
    bool debug;

private:
    static std::string transactionSQL;
    static std::string commitSQL;
    static std::string rollbackSQL;
};

class MysqlConnect : public Base
{
    DECLARE_PRIVATE_D(MysqlConnect);
public:
    CLASS_IS_VALID(MysqlConnect, d->isValid)
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnect)
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(MysqlConnect, Base)

    MysqlConnect (
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname,
        bool debug = false
    );
    ~MysqlConnect () noexcept override = default;

    bool exec (const char *sql);
    MysqlResSetSharedPtr_t execQuery (const char *sql);

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

    inline uint64_t getInsertId ()
    {
        D_PTR(MysqlConnect);
        return mysql_insert_id(d->db);
    }

    inline uint64_t getAffectedRows () const noexcept
    {
        const D_PTR(MysqlConnect);
        return mysql_affected_rows(d->db);
    }

    inline MysqlStmtSharedPtr_t getStmt (const std::string &str)
    {
        D_PTR(MysqlConnect);
        return std::make_shared <MysqlStmt>(d->db, str);
    }

protected:
    explicit MysqlConnect (MysqlConnectPrivate *);
};

#endif //HELLO_WORLD_INCLUDE_MYSQLCONNECT_H_
