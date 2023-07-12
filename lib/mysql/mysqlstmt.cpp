//
// Created by 115282 on 2023/7/3.
//

#include <cstring>
#include "mysql/mysqlstmt.h"

MysqlStmtPrivate::MysqlStmtPrivate (MYSQL *db, const std::string &sql, MysqlStmt *parent)
    : BasePrivate(parent), db(db)
{
    if (!db)
    {
        PRINT_ERROR("params [db] cannot be a nullptr");
        return;
    }
    mysql_ping(db);
    stmt = mysql_stmt_init(db);
    if (!stmt)
    {
        PRINT_MYSQL_STMT_ERROR(stmt, "mysql_stmt_init fail");
        return;
    }
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()))
    {
        PRINT_MYSQL_STMT_ERROR(stmt, "mysql_stmt_prepare fail");
        mysql_stmt_close(stmt);
        stmt = nullptr;
        return;
    }
    paramsCount = mysql_stmt_param_count(stmt);
    if (paramsCount > 0)
    {
        bind = new MYSQL_BIND[paramsCount];
        if (!bind)
        {
            PRINT_ERROR("new MYSQL_BIND[paramsCount] fail");
            mysql_stmt_close(stmt);
            stmt = nullptr;
            return;
        }
        memset(bind, 0, sizeof(MYSQL_BIND) * paramsCount);
    }
}
MysqlStmtPrivate::~MysqlStmtPrivate ()
{
    delete[] bind;
    if (stmt)
        mysql_stmt_close(stmt);
}
MysqlStmt::MysqlStmt (MYSQL *db, const std::string &sql)
    : Base(new MysqlStmtPrivate(db, sql, this)) {}
MysqlStmt::MysqlStmt (BasePrivate *basePrivate)
    : Base(basePrivate) {}
MYSQL_BIND &MysqlStmt::operator[] (size_t index)
{
    D_PTR(MysqlStmt);

    if (index >= d->paramsCount)
        PRINT_ERROR("error range in bind[index], length: %lu, index: %zu", d->paramsCount, index);
    return d->bind[index];
}
const MYSQL_BIND &MysqlStmt::operator[] (size_t index) const
{
    const D_PTR(MysqlStmt);
    if (index >= d->paramsCount)
        PRINT_ERROR("error range in bind[index], length: %lu, index: %zu", d->paramsCount, index);
    return d->bind[index];
}
bool MysqlStmt::execute ()
{
    D_PTR(MysqlStmt);
    if (!d->stmt)
    {
        PRINT_ERROR("no stmt, please check this class is available before call this function");
        return false;
    }
    if (mysql_stmt_bind_param(d->stmt, d->bind))
    {
        PRINT_MYSQL_STMT_ERROR(d->stmt, "mysql_stmt_bind_param fail");
        return false;
    }

    return true;
}
MysqlResSetSharedPtr_t MysqlStmt::executeQuery ()
{
    D_PTR(MysqlStmt);
    if (!execute())
    {
        return nullptr;
    }

    auto p = std::make_shared <MysqlResSet>(d->stmt);

    return p;
}