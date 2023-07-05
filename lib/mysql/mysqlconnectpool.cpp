//
// Created by 115282 on 2023/6/20.
//

#include "mysql/mysqlconnectpool.h"
#include <algorithm>

size_t MysqlConnectPoolPrivate::defaultMinConnectSize = 10;
// 默认过期50秒
std::chrono::milliseconds MysqlConnectPoolPrivate::defaultExpireTime { 50000 };
MysqlConnectPoolPrivate::MysqlConnectPoolPrivate (
    std::string name,
    const char *host,
    unsigned port,
    const char *username,
    const char *password,
    const char *dbname,
    size_t min,
    size_t max,
    std::chrono::milliseconds expireTime,
    MysqlConnectPool *parent
)
    : host(host),
      port(port),
      username(username),
      password(password),
      dbname(dbname),
      minConnectSize(min),
      maxConnectSize(max),
      expireTime(expireTime),
      BasePrivate(parent),
      name(std::move(name)) {}

MysqlConnectPool::MysqlConnectPool (
    const char *poolName,
    const char *host,
    unsigned port,
    const char *username,
    const char *password,
    const char *dbname,
    size_t minConnectSize,
    size_t maxConnectSize,
    std::chrono::milliseconds expireTime
)
    : Base(
    new MysqlConnectPoolPrivate(
        poolName,
        host,
        port,
        username,
        password,
        dbname,
        minConnectSize,
        maxConnectSize,
        expireTime,
        this
    ))
{
    D_PTR(MysqlConnectPool);

    for (int i = 0; i < minConnectSize; ++i)
    {
        auto *p = new MysqlConnect(host, port, username, password, dbname);
        if (!p->isValid())
        {
            delete p;
            break;
        }

        auto *connectWithTime = new MysqlConnectPoolPrivate::MysqlConnectWithTime;
        connectWithTime->mysqlConnect = p;

        d->freeList.push_back(connectWithTime);
    }

    if (d->freeList.size() != minConnectSize)
    {
        d->hasError = true;
        return;
    }

    d->currConnectSize = d->freeList.size();

    auto thread = std::thread(std::bind(&MysqlConnectPool::timerObserver, this));
    thread.detach();
}

MysqlConnectPool::MysqlConnectPool (MysqlConnectPoolPrivate *d)
    : Base(d)
{
    for (int i = 0; i < d->minConnectSize; ++i)
    {
        auto *p = new MysqlConnect(d->host, d->port, d->username, d->password, d->dbname);
        if (!p->isValid())
        {
            delete p;
            break;
        }

        auto *connectWithTime = new MysqlConnectPoolPrivate::MysqlConnectWithTime;
        connectWithTime->mysqlConnect = p;

        d->freeList.push_back(connectWithTime);
    }

    if (d->freeList.size() != d->minConnectSize)
    {
        d->hasError = true;
        return;
    }

    d->currConnectSize = d->freeList.size();

    auto thread = std::thread(std::bind(&MysqlConnectPool::timerObserver, this));
    thread.detach();
}
MysqlConnectPool::~MysqlConnectPool ()
{
    D_PTR(MysqlConnectPool);
    d->terminate.store(true, std::memory_order_relaxed);
    d->cond.notify_all();
    d->timerCond.notify_one();

    for (auto p : d->freeList)
    {
        delete p->mysqlConnect;
        delete p;
    }
}
MysqlConnect *MysqlConnectPool::getConnect (std::chrono::milliseconds time)
{
    D_PTR(MysqlConnectPool);

    if (d->terminate.load(std::memory_order_relaxed))
        return nullptr;

    std::unique_lock <std::mutex> lock(d->mutex);
    if (d->freeList.empty())
    {
        // 如果已经创建到最大值了
        if (d->currConnectSize == d->maxConnectSize)
        {
            // 死等
            if (time.count() < 0)
                d->cond.wait(
                    lock, [&d] {
                        return !d->freeList.empty() || d->terminate;
                    }
                );
                // 轮询
            else if (time.count() == 0)
            {
                lock.unlock();
                while (!d->freeList.empty() || d->terminate)
                    std::this_thread::sleep_for(std::chrono::milliseconds(4));

                lock.lock();
            }
            else
            {
                d->cond.wait_for(
                    lock, time, [&d] {
                        return !d->freeList.empty() || d->terminate;
                    }
                );

                // 如果超时了还是没有空连接
                if (d->freeList.empty())
                    return nullptr;
            }

            if (d->terminate)
                return nullptr;
        }
        else // 如果没有达到最大连接数，创建
        {
            auto *connect = new MysqlConnect(d->host, d->port, d->username, d->password, d->dbname);
            if (!connect->isValid())
            {
                delete connect;
                return nullptr;
            }
            auto *connectWithTime = new MysqlConnectPoolPrivate::MysqlConnectWithTime;

            connectWithTime->mysqlConnect = connect;
            d->freeList.push_back(connectWithTime);
            d->currConnectSize++;
        }
    }

    auto *connect = d->freeList.front();
    connect->lendTime = MysqlConnectPoolPrivate::getNow();
    d->freeList.pop_front();
    d->usedList.push_back(connect);

    return connect->mysqlConnect;
}
bool MysqlConnectPool::retConnect (MysqlConnect *connect)
{
    if (!connect)
    {
        PRINT_ERROR("connect params cannot be nullptr\n");
        return false;
    }

    D_PTR(MysqlConnectPool);

    if (d->usedList.empty())
    {
        PRINT_ERROR("double return connect\n");
        return false;
    }

    std::unique_lock <std::mutex> lock(d->mutex);
    auto it = std::find_if(
        d->usedList.begin(),
        d->usedList.end(),
        [&connect] (MysqlConnectPoolPrivate::MysqlConnectWithTime *connectWithTime) {
            return connect == connectWithTime->mysqlConnect;
        }
    );
    if (it == d->usedList.end())
    {
        PRINT_ERROR("double return connect\n");
        return false;
    }

    (*it)->retTime = MysqlConnectPoolPrivate::getNow();
    d->freeList.push_back(*it);
    d->usedList.erase(it);
    d->cond.notify_one();

    return true;
}
void MysqlConnectPool::timerObserver ()
{
    D_PTR(MysqlConnectPool);
    auto lendIt = d->usedList.end();
    auto retIt = d->freeList.end();
    bool isOperation = false;
    std::chrono::milliseconds wait;

    std::unique_lock <std::mutex> lock(d->mutex);

    while (!d->terminate)
    {
        if (d->minConnectSize == d->currConnectSize && !d->terminate)
        {
            d->timerCond.wait_for(
                lock, d->expireTime, [&d] {
                    return d->minConnectSize != d->currConnectSize || d->terminate;
                }
            );

            if (d->terminate)
                return;
        }

        // 如果有额外的空闲链接
        if (d->freeList.size() > d->minConnectSize)
        {
            retIt = std::min_element(
                d->freeList.begin(),
                d->freeList.end(),
                [] (
                    MysqlConnectPoolPrivate::MysqlConnectWithTime *a,
                    MysqlConnectPoolPrivate::MysqlConnectWithTime *b
                ) {
                    return a->retTime < b->retTime;
                }
            );
            auto now = MysqlConnectPoolPrivate::getNow();
            // 如果当前时间-超时时间 > 归还的时间
            if ((now - d->expireTime) > (*retIt)->retTime)
            {
                d->freeList.erase(retIt);
                delete *retIt;
                d->currConnectSize--;
                isOperation = true;
            }
        }

        // 检测是否有超时未归还
        if (!d->usedList.empty())
        {
            lendIt = std::min_element(
                d->usedList.begin(),
                d->usedList.end(),
                [] (
                    MysqlConnectPoolPrivate::MysqlConnectWithTime *a,
                    MysqlConnectPoolPrivate::MysqlConnectWithTime *b
                ) {
                    return a->lendTime < b->lendTime;
                }
            );

            auto now = MysqlConnectPoolPrivate::getNow();
            // 如果当前时间-超时时间 > 借出的时间
            if ((now - d->expireTime) > (*lendIt)->lendTime)
            {
                // 强制回收
                (*lendIt)->retTime = now;
                d->usedList.erase(lendIt);
                d->freeList.push_back(*lendIt);
                isOperation = true;
            }
        }

        if (isOperation)
            // 如果有操作，直接continue，进入到下一次循环重新操作
            continue;
        else
        {
            if (*retIt && *lendIt)
                wait = (*retIt)->retTime < (*lendIt)->lendTime
                       ? (*retIt)->retTime
                       : (*lendIt)->lendTime;
            else if (!*retIt)
                wait = (*lendIt)->lendTime;
            else if (!*lendIt)
                wait = (*retIt)->retTime;
            else
                continue;

            d->timerCond.wait_for(lock, MysqlConnectPoolPrivate::getNow() - d->expireTime - wait);
        }
    }
}
