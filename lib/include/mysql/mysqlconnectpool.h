//
// Created by 115282 on 2023/6/20.
//

#ifndef HELLO_WORLD_INCLUDE_MYSQLCONNECTPOOL_H_
#define HELLO_WORLD_INCLUDE_MYSQLCONNECTPOOL_H_

#include "mysqlconnect.h"
#include <list>
#include <thread>
#include <condition_variable>
#include <atomic>

class MysqlConnectPool;
class MysqlConnectPoolPrivate : public BasePrivate
{
    static size_t defaultMinConnectSize;
    static std::chrono::milliseconds defaultExpireTime;

    struct MysqlConnectWithTime
    {
        MysqlConnect *mysqlConnect;
        std::chrono::milliseconds lendTime = getNow();
        std::chrono::milliseconds retTime = getNow();
    };
    DECLARE_PUBLIC_Q(MysqlConnectPool);
    using ConnectList = std::list <MysqlConnectWithTime *>;

    static std::chrono::milliseconds getNow ()
    {
        return std::chrono::duration_cast <std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }
public:
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnectPoolPrivate)

    MysqlConnectPoolPrivate (
        std::string name,
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname,
        size_t minConnectSize,
        size_t maxConnectSize,
        std::chrono::milliseconds expireTime,
        MysqlConnectPool *parent
    );
protected:
    std::string name;
    ConnectList freeList;
    ConnectList usedList;
    size_t currConnectSize = 0;
    size_t maxConnectSize;
    size_t minConnectSize;
    std::chrono::milliseconds expireTime;
    std::mutex mutex;
    std::condition_variable cond;
    std::condition_variable timerCond;
    std::atomic <bool> terminate { false };
    bool hasError = false;
    const char *host;
    unsigned port;
    const char *username;
    const char *password;
    const char *dbname;
};

class MysqlConnectPool : public Base
{
    DECLARE_PRIVATE_D(MysqlConnectPool);
public:
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(MysqlConnectPool, Base)
    CLASS_COPY_CONSTRUCTOR_DISABLED(MysqlConnectPool)
    CLASS_IS_VALID(MysqlConnectPool, d->hasError)

    MysqlConnectPool (
        const char *poolName,
        const char *host,
        unsigned port,
        const char *username,
        const char *password,
        const char *dbname,
        size_t minConnectSize = MysqlConnectPoolPrivate::defaultMinConnectSize,
        size_t maxConnectSize = MysqlConnectPoolPrivate::defaultMinConnectSize * 2,
        std::chrono::milliseconds expireTime = MysqlConnectPoolPrivate::defaultExpireTime
    );

    ~MysqlConnectPool () override;

    /*
     * -1 sleep and wait
     * 0 polling
     * > 0 wait time
     * */
    MysqlConnect *getConnect (std::chrono::milliseconds time = std::chrono::milliseconds(-1));

    bool retConnect (MysqlConnect *);

protected:
    explicit MysqlConnectPool (MysqlConnectPoolPrivate *);

    void timerObserver ();
};

#endif //HELLO_WORLD_INCLUDE_MYSQLCONNECTPOOL_H_
