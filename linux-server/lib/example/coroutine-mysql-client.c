//
// Created by Administrator on 2023/4/3.
//
#include <mysql.h>
#include "coroutine.h"

#define HOST "192.168.200.100"
#define USER "root"
#define PASSWORD "jianv4as"
#define DATABASE_NAME "YOSHIKI_DB"
#define PORT 3306
void mysqlClient (void *arg)
{
    MYSQL *mysql = mysql_init(NULL);
    if (!mysql)
    {
        printf("mysql_init error\n");
        return;
    }

    if (!mysql_real_connect(
        mysql,
        HOST,
        USER,
        PASSWORD,
        DATABASE_NAME,
        PORT,
        NULL,
        0
    ))
        printf("mysql_real_connect error : %s\n", mysql_error(mysql));
    else
        printf("mysql_real_connect success\n");
}
int main ()
{
    setbuf(stdout, 0);
    initHook();

    Coroutine *coroutine = NULL;
    coroutineCreate(&coroutine, mysqlClient, 0);

    coroutineScheduleRun();
}