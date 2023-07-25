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

class TimerNode;
class TimerNodePrivate : BasePrivate
{
    DECLARE_PUBLIC_Q(TimerNode);
    using Callback = std::function <void ()>;

protected:
    TimerNodePrivate (
        Callback &&callback,
        std::chrono::milliseconds expire,
        uint32_t id,
        TimerNode *parent
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
    uint32_t id;
    Callback callback;
    std::chrono::milliseconds expire;
};
class TimerNode : Base, Utils::NonAbleCopy
{
    DECLARE_PRIVATE_D(TimerNode);
protected:
    TimerNode (TimerNodePrivate::Callback &&callback, std::chrono::milliseconds expire)
        : Base(new TimerNodePrivate(std::move(callback), expire, id++, this)) {}
    ~TimerNode () noexcept override = default;

    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(TimerNode, Base)
    CLASS_IS_VALID(TimerNode, true)

    bool operator< (const TimerNode &other) const noexcept
    {
        const D_PTR(TimerNode);

        return *d < *other.d_fun();
    }

protected:
    static uint32_t id;
};

class Timeout;
class TimeoutPrivate : BasePrivate
{
    DECLARE_PUBLIC_Q(Timeout);

protected:
    TimeoutPrivate () = default;
    ~TimeoutPrivate () noexcept override = default;

private:
    std::set <TimerNode> timeSet;
};

class Timeout : Base, Utils::NonAbleCopy
{
    DECLARE_PRIVATE_D(Timeout);

public:
    Timeout () = default;
    ~Timeout () noexcept override = default;
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(Timeout, Base)
    CLASS_IS_VALID(Timeout, true)
};

uint32_t TimerNode::id = 0;

TimerNodePrivate::TimerNodePrivate (
    TimerNodePrivate::Callback &&callback,
    std::chrono::milliseconds expire,
    uint32_t id,
    TimerNode *parent
)
    : callback(std::move(callback)), expire(expire), id(id), BasePrivate(parent) {}
#endif //LINUX_SERVER_LIB_INCLUDE_TIMEOUT_H_
