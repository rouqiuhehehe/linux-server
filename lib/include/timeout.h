//
// Created by 115282 on 2023/7/25.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_
#define LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_

#include <functional>
#include "util/global.h"
#include "util/util.h"
#include <chrono>
#include <set>
#include <thread>
#include <algorithm>
#include <condition_variable>
#include <mutex>

template <class T>
class Timeout;
template <class T>
class TimerNode;
template <class T>
class TimeoutPrivate;
template <class T>
class TimerNodePrivate : BaseTemplatePrivate <T>
{
    DECLARE_TEMPLATE_PUBLIC_Q(TimerNode, T);
    friend Timeout <T>;
    using Callback = std::function <void ()>;
    using TimerId = uint32_t;

public:
    ~TimerNodePrivate () noexcept override = default;

protected:
    TimerNodePrivate (
        Callback &&callback,
        T expire,
        uint32_t id,
        bool keepAlive,
        TimerNode <T> *parent
    );

    inline T getExpireTime () const noexcept
    {
        return expire + beginTime;
    }

    bool operator< (const TimerNodePrivate &other) const noexcept
    {
        if (getExpireTime() < other.getExpireTime())
            return true;
        else if (getExpireTime() > other.getExpireTime())
            return false;
        else
            return id < other.id;

    }

private:
    TimerId id;
    Callback callback;
    T expire;
    T beginTime;
    bool keepAlive;
};

template <class T>
class TimerNode : BaseTemplate <T>, Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_PRIVATE_D(TimerNode, T);
    friend Timeout <T>;
    friend std::set <TimerNode <T>>;
    friend TimeoutPrivate <T>;

public:
    ~TimerNode () noexcept override = default;

protected:
    TimerNode (typename TimerNodePrivate <T>::Callback &&callback, T expire, bool keepAlive)
        : BaseTemplate <T>(
        new TimerNodePrivate <T>(
            std::move(callback),
            expire,
            id++,
            keepAlive,
            this
        )) {}

    explicit TimerNode (TimerNodePrivate <T> *d)
        : BaseTemplate <T>(d) {}

    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(TimerNode, Base)
    CLASS_TEMPLATE_IS_VALID(TimerNode, true, T)

    bool operator< (const TimerNode &other) const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);

        return *d < *other.d_fun();
    }

    inline typename TimerNodePrivate <T>::TimerId getId () const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);
        return d->id;
    }

    inline T getExpireTime () const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);
        return d->getExpireTime();
    }

    inline void setBeginTime (T begin)
    {
        D_TEMPLATE_PTR(TimerNode, T);
        d->beginTime = begin;
    }
    inline typename TimerNodePrivate <T>::Callback getFun () const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);
        return d->callback;
    }

    inline bool isKeepAlive () const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);
        return d->keepAlive;
    }

protected:
    static typename TimerNodePrivate <T>::TimerId id;
};

template <class T>
class TimeoutPrivate : BaseTemplatePrivate <T>
{
    DECLARE_TEMPLATE_PUBLIC_Q(Timeout, T);

protected:
    explicit TimeoutPrivate (Timeout <T> *parent);
    ~TimeoutPrivate () noexcept override
    {
        terminate = true;
        cond.notify_one();
        for (auto v : timeSet)
            delete v;
        timeSet.clear();
    };

private:
    class TimerNodeLess
    {
    public:
        bool operator() (TimerNode <T> *a, TimerNode <T> *b) const noexcept
        {
            return *a < *b;
        }
    };
    std::set <TimerNode <T> *, TimerNodeLess> timeSet;
    std::condition_variable cond;
    bool terminate = false;
};

template <class T = std::chrono::milliseconds>
class Timeout : BaseTemplate <T>, Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_PRIVATE_D(Timeout, T);
    friend TimerNodePrivate <T>;

public:
    using TimerId = typename TimerNodePrivate <T>::TimerId;
    using TimeType = int64_t;
    Timeout ()
        : BaseTemplate <T>(new TimeoutPrivate <T>(this))
    {
        setTimeObserver();
    }
    explicit Timeout (TimeoutPrivate <T> *d)
        : BaseTemplate <T>(d) {}

    ~Timeout () noexcept override = default;
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(Timeout, Base)
    CLASS_TEMPLATE_IS_VALID(Timeout, true, T)

    inline TimerId setTimeout (typename TimerNodePrivate <T>::Callback &&fun, T expire)
    {
        return setTimer(std::forward <typename TimerNodePrivate <T>::Callback>(fun), expire, false);
    }

    inline TimerId setInterval (typename TimerNodePrivate <T>::Callback &&fun, T expire)
    {
        return setTimer(std::forward <typename TimerNodePrivate <T>::Callback>(fun), expire, true);
    }

    inline bool clearTimer (TimerId id)
    {
        D_TEMPLATE_PTR(Timeout, T);

        auto it = std::find_if(
            d->timeSet.begin(), d->timeSet.end(), [&id] (TimerNode <T> *node) {
                return node->getId() == id;
            }
        );
        if (it == d->timeSet.end())
            return false;

        TimerNode <T> *node = *it;
        d->timeSet.erase(it);
        delete node;
        return true;
    }

protected:
    static inline T getCurrentTime () noexcept
    {
        return std::chrono::duration_cast <T>(std::chrono::steady_clock::now().time_since_epoch());
    }
    inline TimerId setTimer (
        typename TimerNodePrivate <T>::Callback &&fun,
        T expire,
        bool keepAlive
    )
    {
        D_TEMPLATE_PTR(Timeout, T);

        auto *timeNode = new TimerNode <T>(std::move(fun), expire, keepAlive);
        d->timeSet.emplace(timeNode);
        d->cond.notify_one();

        return timeNode->getId();
    }

    inline void setTimeObserver ()
    {
        std::thread timeObserver(&Timeout::timeObserver, this);

        auto coreNum = std::thread::hardware_concurrency();
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(coreNum - 1, &cpuSet);
        pthread_t threadId = timeObserver.native_handle();
        if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuSet) != 0)
            std::cerr << "Failed to set thread affinity : " << strerror(errno) << std::endl;

        timeObserver.detach();
    }
    inline void timeObserver ()
    {
        D_TEMPLATE_PTR(Timeout, T);
        std::mutex mutex;
        std::unique_lock <std::mutex> lock(mutex);
        T currentTime;

        for (;;)
        {
            if (d->terminate) return;
            if (d->timeSet.empty())
            {
                d->cond.wait(
                    lock, [&d] {
                        return !d->timeSet.empty() || d->terminate;
                    }
                );

                if (d->terminate)
                    return;
            }

            auto minIt = d->timeSet.begin();
            TimerNode <T> *node = *minIt;
            currentTime = getCurrentTime();

            if (node->getExpireTime() <= currentTime)
            {
                node->getFun()();

                // terminate 退出
                if (d->terminate)
                    return;

                d->timeSet.erase(minIt);
                // setInterval
                if (node->isKeepAlive())
                {
                    node->setBeginTime(getCurrentTime());
                    d->timeSet.emplace(node);
                }
            }
            else
            {
                T diff = node->getExpireTime() - currentTime;

                bool res = d->cond.wait_for(
                    lock, diff, [&d] {
                        return d->terminate;
                    }
                );
                // terminate 退出
                if (res)
                    return;
            }
        }
    }
};

template <class T>
typename TimerNodePrivate <T>::TimerId TimerNode <T>::id = 0;
template <class T>
TimerNodePrivate <T>::TimerNodePrivate (
    TimerNodePrivate::Callback &&callback,
    T expire,
    uint32_t id,
    bool keepAlive,
    TimerNode <T> *parent
)
    : callback(std::move(callback)),
      expire(expire),
      id(id),
      keepAlive(keepAlive),
      beginTime(Timeout <T>::getCurrentTime()),
      BaseTemplatePrivate <T>(parent) {}
template <class T>
TimeoutPrivate <T>::TimeoutPrivate (Timeout <T> *parent)
    : BaseTemplatePrivate <T>(parent) {};

#endif //LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_
