//
// Created by 115282 on 2023/9/22.
//

#include <mysql.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <thread>

#define THREAD_MAX_NUM 10

#define HOST "127.0.0.1"
#define PORT 3306
#define USERNAME "root"
#define PASSWORD "jianv4as"
#define DBNAME "yoshiki_db"

#define TABLE_NAME "NewTable"
#define UPDATE_SQL "update " TABLE_NAME " set `count1` = `count1` - 1 where `id` = 1 and `count1` > 0"

#define CHECK_RET(expr, fnName)                                                 \
            if (expr)                                                           \
            {                                                                   \
                fprintf(stderr,"%s:%s:%d:  "#fnName" err %s\n",                 \
                         __FILE__, __FUNCTION__, __LINE__, strerror(errno));    \
                exit(EXIT_FAILURE);                                             \
            }

int main ()
{
    std::thread threads[THREAD_MAX_NUM];
    for (auto &i : threads)
    {
        i = std::thread(
            [] {
                MYSQL *db = mysql_init(nullptr);
                CHECK_RET(!db, mysql_init);

                if (!mysql_real_connect(db, HOST, USERNAME, PASSWORD, DBNAME, PORT, nullptr, 0))
                {
                    std::cerr << "mysql_real_connect error : " << mysql_error(db);
                    exit(EXIT_FAILURE);
                }
                uint64_t size;
                do
                {
                    if (mysql_real_query(db, UPDATE_SQL, strlen(UPDATE_SQL)) == 0)
                    {
                        size = mysql_affected_rows(db);
                        std::cout << size << std::endl;
                    }
                    else
                    {
                        std::cerr << "mysql_real_query error : " << mysql_error(db);
                        exit(EXIT_FAILURE);
                    }
                } while (size > 0);
            }
        );
    }
    for (auto &i : threads)
    {
        i.join();
    }

    return 0;
}