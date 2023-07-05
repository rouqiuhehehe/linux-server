//
// Created by 115282 on 2023/6/20.
//

#include "mysql/mysqlresult.h"
#include <algorithm>
#include <sstream>

MysqlResSetPrivate::MysqlResSetPrivate (MYSQL_RES *res, MysqlResSet *parent)
    : BasePrivate(parent)
{
    if (res)
    {
        MYSQL_FIELD *field = mysql_fetch_fields(res);
        if (field)
        {
            for (uint64_t i = 0; i < mysql_num_fields(res); ++i)
                fields.emplace_back(field[i].name);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                ResMapType map;
                int i = 0;
                for (const std::string &fieldName : fields)
                    map.insert(std::make_pair(fieldName, row[i++]));

                results.push_back(std::move(map));
            }

            isValid = true;
            mysql_free_result(res);
        }
    }
}
std::ostream &operator<< (std::ostream &os, const MysqlResSetPrivate *rhs)
{
    std::stringstream stream;
    stream << "[";
    int i = 0;
    for (const auto &resMap : rhs->results)
    {
        stream << "{";
        for (const auto &pair : resMap)
        {
            stream << "\"" << pair.first << "\":\"" << pair.second << "\",";
            if (++i > 5)
            {
                stream << '\n';
                i = 0;
            }
        }
        stream.seekp(-1, std::ios_base::cur);
        stream << "},\n";
        i = 0;
    }
    stream.seekp(-2, std::ios_base::cur);
    stream << "]" << std::endl;

    os << stream.str();
    return os;
}

MysqlResSet::MysqlResSet (MYSQL_RES *res)
    : Base(new MysqlResSetPrivate(res, this)) {}
MysqlResSet::MysqlResSet (MysqlResSetPrivate *d)
    : Base(d) {}