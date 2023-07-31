//
// Created by 115282 on 2023/7/3.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLSTMT_H_
#define HELLO_WORLD_INCLUDE_MYSQLSTMT_H_

#include "mysqlresult.h"

class MysqlStmt;
class MysqlStmtPrivate : BasePrivate
{
    DECLARE_PUBLIC_Q(MysqlStmt);
protected:
    MysqlStmtPrivate (MYSQL *db, const std::string &sql, MysqlStmt *parent);
    ~MysqlStmtPrivate () override;

    MysqlStmtPrivate (MysqlStmtPrivate &&rhs) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        this->operator=(std::move(rhs));
    }
    MysqlStmtPrivate &operator= (MysqlStmtPrivate &&rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        stmt = rhs.stmt;
        bind = rhs.bind;
        db = rhs.db;
        paramsCount = rhs.paramsCount;
        rhs.stmt = nullptr;
        rhs.bind = nullptr;
        rhs.db = nullptr;

        return *this;
    }

    MYSQL_STMT *stmt;
    MYSQL_BIND *bind = nullptr;
    MYSQL *db;
    // 预处理语句（prepared statement）参数的数量
    unsigned long paramsCount;
};
class MysqlStmt : Base
{
    DECLARE_PRIVATE_D(MysqlStmt);

public:
    CLASS_IS_VALID(MysqlStmt, d->stmt);

    MysqlStmt (MYSQL *db, const std::string &sql);
    ~MysqlStmt () noexcept override = default;
    MysqlStmt (MysqlStmt &&rhs) noexcept
    {
        this->operator=(std::move(rhs));
    }
    MysqlStmt &operator= (MysqlStmt &&rhs) noexcept
    {
        d_fun()->operator=(std::move(*rhs.d_fun()));

        return *this;
    }

    bool execute ();
    MysqlResSetSharedPtr_t executeQuery ();

    MYSQL_BIND &operator[] (size_t index);
    const MYSQL_BIND &operator[] (size_t index) const;

    inline void setParam (size_t index, unsigned char &c);
    inline void setParam (size_t index, short &c);
    inline void setParam (size_t index, INT24 &c);
    inline void setParam (size_t index, int32_t &c);
    inline void setParam (size_t index, int64_t &c);
    inline void setParam (size_t index, float &c);
    inline void setParam (size_t index, double &c);
    inline void setParam (size_t index, const std::string &c);
    inline void setParam (size_t index, char *c, size_t len);

    inline uint64_t getStmtInsertId ();
    inline MYSQL_STMT *getStmt ();
    inline const MYSQL_STMT *getStmt () const;

protected:
    explicit MysqlStmt (BasePrivate *basePrivate);
};
void MysqlStmt::setParam (size_t index, unsigned char &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_TINY;
    bind.buffer_length = sizeof(char);
}
void MysqlStmt::setParam (size_t index, short &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_SHORT;
    bind.buffer_length = sizeof(short);
}
void MysqlStmt::setParam (size_t index, INT24 &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_INT24;
    bind.buffer_length = sizeof(INT24);
}
void MysqlStmt::setParam (size_t index, int32_t &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer_length = sizeof(int32_t);
}
void MysqlStmt::setParam (size_t index, int64_t &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer_length = sizeof(int64_t);
}
void MysqlStmt::setParam (size_t index, float &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_FLOAT;
    bind.buffer_length = sizeof(float);
}
void MysqlStmt::setParam (size_t index, double &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = &c;
    bind.buffer_type = MYSQL_TYPE_DOUBLE;
    bind.buffer_length = sizeof(double);
}
void MysqlStmt::setParam (size_t index, const std::string &c)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = const_cast<char *>(c.c_str());
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer_length = c.size();
}
void MysqlStmt::setParam (size_t index, char *c, size_t len)
{
    MYSQL_BIND &bind = this->operator[](index);
    bind.buffer = c;
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer_length = len;
}
uint64_t MysqlStmt::getStmtInsertId ()
{
    D_PTR(MysqlStmt);
    return mysql_stmt_insert_id(d->stmt);
}
MYSQL_STMT *MysqlStmt::getStmt ()
{
    D_PTR(MysqlStmt);
    return d->stmt;
}
const MYSQL_STMT *MysqlStmt::getStmt () const
{
    const D_PTR(MysqlStmt);
    return d->stmt;
}

using MysqlStmtSharedPtr_t = std::shared_ptr <MysqlStmt>;

#endif //HELLO_WORLD_INCLUDE_MYSQLSTMT_H_
