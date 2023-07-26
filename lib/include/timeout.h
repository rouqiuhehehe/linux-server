//
// Created by 115282 on 2023/7/25.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_
#define LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_

#include <functional>
#include "global.h"
#include "util.h"
#include <chrono>
#include <set>
#include <thread>
#include <algorithm>
#include <condition_variable>

template <class T>
class Timeout;
template <class T>
class TimerNode;
template <class T>
class TimerNodePrivate : BaseTemplatePrivate <T>
{
    DECLARE_TEMPLATE_PUBLIC_Q(TimerNode, T);
    friend Timeout <T>;
    using Callback = std::function <void ()>;
    using TimerId = uint32_t;

protected:
    TimerNodePrivate (
        Callback &&callback,
        T expire,
        uint32_t id,
        bool keepAlive,
        TimerNode <T> *parent
    );
    ~TimerNodePrivate () noexcept override = default;

    bool operator< (const TimerNodePrivate &other) const noexcept
    {
        if (expire < other.expire)
            return true;
        else if (expire > other.expire)
            return false;
        else
            return id < other.id;
    }

private:
    TimerId id;
    Callback callback;
    T expire;
    bool keepAlive;
};

template <class T>
class TimerNode : BaseTemplate <T>, Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_PRIVATE_D(TimerNode, T);
    friend Timeout <T>;

protected:
    TimerNode (typename TimerNodePrivate <T>::Callback &&callback, T expire)
        : BaseTemplate <T>(new TimerNodePrivate <T>(std::move(callback), expire, id++, this)) {}

    explicit TimerNode (TimerNodePrivate <T> *d)
        : BaseTemplate <T>(d) {}
    ~TimerNode () noexcept override = default;

    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(TimerNode, Base)
    CLASS_TEMPLATE_IS_VALID(TimerNode, true, T)

    bool operator< (const TimerNode &other) const noexcept
    {
        const D_TEMPLATE_PTR(TimerNode, T);

        return *d < *other.d_fun();
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
    ~TimeoutPrivate () noexcept override = default;

private:
    std::set <TimerNode <T> *> timeSet;
    std::condition_variable cond;
};

template <class T = std::chrono::milliseconds>
class Timeout : BaseTemplate <T>, Utils::NonAbleCopy
{
    DECLARE_TEMPLATE_PRIVATE_D(Timeout, T);

public:
    using TimerId = typename TimerNodePrivate <T>::TimerId;
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
        setTimer(std::forward <typename TimerNodePrivate <T>::Callback>(fun), expire, false);
    }

    inline TimerId setInterval (typename TimerNodePrivate <T>::Callback &&fun, T expire)
    {
        setTimer(std::forward <typename TimerNodePrivate <T>::Callback>(fun), expire, true);
    }

    inline bool clearTimer (TimerId id)
    {
        D_TEMPLATE_PTR(Timeout, T);

        auto it = std::find_if(
            d->timeSet.begin(), d->timeSet.end(), [&id] (const TimerNode <T> &node) {
                return node.d_fun()->id == id;
            }
        );
        if (it == d->timeSet.end())
            return false;

        d->timeSet.erase(it);
        return true;
    }

protected:
    inline int64_t getCurrentTime () const noexcept
    {
        return std::chrono::duration_cast <T>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    inline TimerId setTimer (
        typename TimerNodePrivate <T>::Callback &&fun,
        T expire,
        bool keepAlive
    )
    {
        D_TEMPLATE_PTR(Timeout, T);

        auto *timeNode = new TimerNode <T>(std::move(fun), expire + getCurrentTime(), keepAlive);
        d->timeSet.emplace(timeNode);
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

        if (d->timeSet.empty())
        {

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
      BaseTemplatePrivate <T>(parent) {}
template <class T>
TimeoutPrivate <T>::TimeoutPrivate (Timeout <T> *parent)
    : BaseTemplatePrivate <T>(parent) {};
#endif //LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_
