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
#define WHERE_QUERY " where sex = ? and id between ? and ?"
#define QUERY_SQL "select * from " TABLE_NAME WHERE_QUERY

int main ()
{
    MysqlConnectPool mysqlConnectPool(
        "ddsa",
        HOST,
        PORT,
        USERNAME,
        PASSWORD,
        DBNAME,
        true
    );

    auto db = mysqlConnectPool.getConnect();
    // auto db1 = mysqlConnectPool.getConnect();
    // auto db2 = mysqlConnectPool.getConnect();
    // auto db3 = mysqlConnectPool.getConnect();
    // auto db4 = mysqlConnectPool.getConnect();
    // auto db5 = mysqlConnectPool.getConnect();
    // auto db6 = mysqlConnectPool.getConnect();
    // auto db7 = mysqlConnectPool.getConnect();
    // auto db8 = mysqlConnectPool.getConnect();
    // auto db9 = mysqlConnectPool.getConnect();
    // MysqlConnect db(
    //     HOST,
    //     PORT,
    //     USERNAME,
    //     PASSWORD,
    //     DBNAME
    // );

    // auto res = db->execQuery(QUERY_SQL);

    // std::cout << *res << std::endl;
    auto stmt = db->getStmt(QUERY_SQL);

    int left = 1, right = 100;
    if (stmt->isValid())
    {
        stmt->setParam(0, "man");
        stmt->setParam(1, left);
        stmt->setParam(2, right);

        auto res = stmt->executeQuery();

        if (res->isValid())
        {
            std::cout << *res << std::endl;
        }
    }

    // mysqlConnectPool.retConnect(db3);
    return 0;
}