//
// Created by 115282 on 2023/6/20.
//

#include "mysql/mysqlconnect.h"
#include <cstring>

std::string MysqlConnectPrivate::transactionSQL = "start transaction\n"; // NOLINT
std::string MysqlConnectPrivate::commitSQL = "commit\n"; // NOLINT
std::string MysqlConnectPrivate::rollbackSQL = "rollback\n"; // NOLINT

MysqlConnectPrivate::MysqlConnectPrivate (
    const char *host,
    unsigned int port,
    const char *username,
    const char *password,
    const char *dbname,
    bool debug,
    MysqlConnect *parent
)
    : BasePrivate(parent), db(mysql_init(nullptr)), debug(debug)
{
    if (!db)
    {
        PRINT_MYSQL_ERROR(db, "mysql_init error");
        return;
    }
    unsigned option = 1;
    // 启动自动重连
    mysql_options(db, MYSQL_OPT_RECONNECT, &option);
    mysql_options(db, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(db, host, username, password, dbname, port, nullptr, 0))
    {
        PRINT_MYSQL_ERROR(db, "mysql_real_connect error");
        return;
    }
    isValid = true;
}

MysqlConnectPrivate::~MysqlConnectPrivate () noexcept
{
    if (db)
    {
        mysql_close(db);
        db = nullptr;
    }
}

MysqlConnect::MysqlConnect (MysqlConnectPrivate *d)
    : Base(d) {}
MysqlConnect::MysqlConnect (
    const char *host,
    unsigned int port,
    const char *username,
    const char *password,
    const char *dbname,
    bool debug
)
    : Base(new MysqlConnectPrivate(host, port, username, password, dbname, debug, this)) {}
bool MysqlConnect::exec (const char *sql)
{
    D_PTR(MysqlConnect);
    // ping一下mysql 断开了可以自动重连
    mysql_ping(d->db);

    if (d->debug)
        PRINT_INFO("%s", sql);

    if (mysql_real_query(d->db, sql, std::strlen(sql)))
    {
        PRINT_MYSQL_ERROR(d->db, "mysql_real_query");
        return false;
    }

    return true;
}
MysqlResSetSharedPtr_t MysqlConnect::execQuery (const char *sql)
{
    D_PTR(MysqlConnect);
    mysql_ping(d->db);

    if (d->debug)
        PRINT_INFO("%s", sql);

    if (mysql_real_query(d->db, sql, std::strlen(sql)))
    {
        PRINT_MYSQL_ERROR(d->db, "mysql_real_query");
        return {};
    }

    MYSQL_RES *res = mysql_store_result(d->db);
    if (!res)
    {
        PRINT_MYSQL_ERROR(d->db, "mysql_store_result");
        return {};
    }

    return std::make_shared <MysqlResSet>(res);
}
bool MysqlConnect::transaction ()
{
    D_PTR(MysqlConnect);
    mysql_ping(d->db);

    if (mysql_real_query(
        d->db,
        MysqlConnectPrivate::transactionSQL.c_str(),
        MysqlConnectPrivate::transactionSQL.size()))
    {
        PRINT_MYSQL_ERROR(d->db, "transaction");
        return false;
    }
    return true;
}
bool MysqlConnect::rollback ()
{
    D_PTR(MysqlConnect);
    mysql_ping(d->db);

    if (mysql_real_query(
        d->db,
        MysqlConnectPrivate::rollbackSQL.c_str(),
        MysqlConnectPrivate::rollbackSQL.size()))
    {
        PRINT_MYSQL_ERROR(d->db, "rollback");
        return false;
    }
    return true;
}
bool MysqlConnect::commit ()
{
    D_PTR(MysqlConnect);
    mysql_ping(d->db);

    if (mysql_real_query(
        d->db,
        MysqlConnectPrivate::commitSQL.c_str(),
        MysqlConnectPrivate::commitSQL.size()))
    {
        PRINT_MYSQL_ERROR(d->db, "commit");
        return false;
    }
    return true;
}
