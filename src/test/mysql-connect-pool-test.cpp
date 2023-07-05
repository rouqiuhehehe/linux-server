//
// Created by Yoshiki on 2023/6/22.
//

#include "mysql/mysqlconnectpool.h"

#define HOST "127.0.0.1"
#define PORT 3306
#define USERNAME "root"
#define PASSWORD "jianv4as"
#define DBNAME "DB_Yoshiki"

#define TABLE_NAME "test_table"
#define QUERY_SQL "select * from " TABLE_NAME

int main ()
{
    MysqlConnectPool mysqlConnectPool(
        "ddsa",
        HOST,
        PORT,
        USERNAME,
        PASSWORD,
        DBNAME
    );

    auto db = mysqlConnectPool.getConnect();
    auto res = db->execQuery(QUERY_SQL);

    std::cout << res.getResults().size() << std::endl;

    return 0;
}