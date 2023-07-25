//
// Created by Yoshiki on 2023/6/22.
//

#include "mysql/mysqlconnectpool.h"
#include "threadpoolexecutor.h"

#define HOST "127.0.0.1"
#define PORT 3306
#define USERNAME "root"
#define PASSWORD "jianv4as"
#define DBNAME "yoshiki_db"

#define TABLE_NAME "u_test"
#define WHERE_QUERY " where u_sex = ? and u_id between ? and ?"
#define QUERY_SQL "select * from " TABLE_NAME WHERE_QUERY

#define CHECK_IS_VALID(d) if (!d->isValid()) exit(EXIT_FAILURE)

int main ()
{
    ThreadPoolExecutor *threadPool = ThreadPoolExecutor::getInstance();
    CHECK_IS_VALID(threadPool);

    MysqlConnectPool mysqlConnectPool(
        "ddsa",
        HOST,
        PORT,
        USERNAME,
        PASSWORD,
        DBNAME,
        true
    );
    CHECK_IS_VALID((&mysqlConnectPool));

    auto fun = [&mysqlConnectPool] (const std::string &sex, int left, int right) {
        auto db = mysqlConnectPool.getConnect();
        auto stmt = db->getStmt(QUERY_SQL);

        if (stmt->isValid())
        {
            stmt->setParam(0, sex);
            stmt->setParam(1, left);
            stmt->setParam(2, right);

            auto res = stmt->executeQuery();

            if (res->isValid())
            {
                std::cout << *res << std::endl;
            }
        }

        mysqlConnectPool.retConnect(db);
    };

    threadPool->exec(fun, "man", 5000, 5100);
    threadPool->exec(fun, "woman", 6000, 6100);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}