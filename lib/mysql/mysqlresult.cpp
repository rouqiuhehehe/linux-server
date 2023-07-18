#include "mysql/mysqlresult.h"
#include <algorithm>
#include <cstring>

MysqlResSetPrivate::MysqlResSetPrivate (MYSQL_RES *res, MysqlResSet *parent)
    : BasePrivate(parent)
{
    MYSQL_FIELD *field = mysql_fetch_fields(res);
    uint32_t fieldNum = mysql_num_fields(res);
    if (field)
    {
        for (uint32_t i = 0; i < fieldNum; ++i)
            fields.emplace_back(field[i].name);

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)))
            insertResMap(row);

        isValid = true;
        mysql_free_result(res);
    }
}
MysqlResSetPrivate::MysqlResSetPrivate (MYSQL_STMT *stmt, MysqlResSet *parent)
    : BasePrivate(parent)
{
    if (!stmt)
        return;

    MYSQL_RES *res = mysql_stmt_result_metadata(stmt);
    MYSQL_FIELD *field = mysql_fetch_fields(res);
    uint32_t fieldNum = mysql_num_fields(res);

    auto *bind = static_cast<MYSQL_BIND *>(::operator new(fieldNum * sizeof(MYSQL_BIND)));
    std::memset(bind, 0, fieldNum * sizeof(MYSQL_BIND));

    if (field)
    {
        for (uint32_t i = 0; i < fieldNum; ++i)
        {
            fields.emplace_back(field[i].name);
            setResultBind(&bind[i], &field[i]);
        }
        // 缓存所有结果集，默认情况下，成功执行的准备语句的结果集不会在客户端缓冲，而是 mysql_stmt_fetch()从服务器一次获取一个
        // mysql_stmt_data_seek()、 mysql_stmt_row_seek()或 mysql_stmt_row_tell()。这些函数需要可查找的结果集
        // if (mysql_stmt_store_result(stmt))
        // {
        //     PRINT_MYSQL_STMT_ERROR(stmt, "mysql_stmt_store_result fail");
        // }
        if (mysql_stmt_bind_result(stmt, bind))
        {
            PRINT_ERROR("mysql_stmt_bind_result error : %s", mysql_stmt_error(stmt));
            goto error;
        }
        // if (mysql_stmt_execute(stmt))
        // {
        //     PRINT_MYSQL_STMT_ERROR(stmt, "mysql_stmt_execute fail");
        //     goto error;
        // }

        ResMapType map;
        int ret;
        while ((ret = mysql_stmt_fetch(stmt)) != MYSQL_NO_DATA && ret != 1)
        {
            map.clear();
            if (ret == MYSQL_DATA_TRUNCATED)
                PRINT_WARNING("mysql_stmt_fetch data truncated");

            int i = 0;

            std::string buffer;
            for (const std::string &fieldName : fields)
            {
                buffer = std::move(MysqlResSet::mysqlBindTypeToString(&bind[i++]));
                map.insert(std::make_pair(fieldName, buffer));
            }

            results.push_back(std::move(map));
        }

        isValid = true;
    }

error:
    mysql_free_result(res);
    // mysql_stmt_free_result(stmt);
    for (int i = 0; i < fieldNum; ++i)
        ::operator delete(bind[i].buffer);
    delete[] bind;
}
void MysqlResSetPrivate::insertResMap (MYSQL_ROW row)
{
    ResMapType map;
    int i = 0;
    for (const std::string &fieldName : fields)
        map.insert(std::make_pair(fieldName, row[i++]));

    results.push_back(std::move(map));
}
void MysqlResSetPrivate::setResultBind (MYSQL_BIND *bind, MYSQL_FIELD *field)
{
    bind->buffer_type = field->type;
    if (bind->buffer_type == MYSQL_TYPE_DATETIME || bind->buffer_type == MYSQL_TYPE_TIMESTAMP
        || bind->buffer_type == MYSQL_TYPE_DATE || bind->buffer_type == MYSQL_TYPE_TIME)
        bind->buffer = ::operator new(sizeof(MYSQL_TIME));
    else
        bind->buffer = ::operator new(field->length);
    // MYSQL_TYPE_VAR_STRING需要设置缓冲区大小
    if (field->type == MYSQL_TYPE_VAR_STRING)
        bind->buffer_length = field->length;
    bind->length = &bind->buffer_length;
}
std::ostream &operator<< (std::ostream &os, const MysqlResSetPrivate *rhs)
{
    std::stringstream stream;
    stream << "[";
    for (const auto &resMap : rhs->results)
    {
        stream << "{";
        for (const auto &pair : resMap)
        {
            stream << "\"" << pair.first << "\":\"" << pair.second << "\",";
        }
        stream.seekp(-1, std::ios_base::cur);
        stream << "},\n";
    }
    if (!rhs->results.empty())
        stream.seekp(-2, std::ios_base::cur);
    stream << "]" << std::endl;

    os << stream.str();
    return os;
}

MysqlResSet::MysqlResSet (MYSQL_RES *res)
    : Base(new MysqlResSetPrivate(res, this)) {}
MysqlResSet::MysqlResSet (MYSQL_STMT *stmt)
    : Base(new MysqlResSetPrivate(stmt, this)) {}
MysqlResSet::MysqlResSet (MysqlResSetPrivate *d)
    : Base(d) {}
