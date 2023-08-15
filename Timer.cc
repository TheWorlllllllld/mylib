#include "Timer.h"

std::atomic<int64_t> Timer::numCreated_;

// 用于计算下一个超时时刻
void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        // 如果是重复的定时器，那么下一个超时时刻就是当前时间加上定时时长
        expiration_ = addTimer(now, interval_);
    }
    else
    {
        // 如果不是重复的定时器，那么下一个超时时刻就是无效的时间
        expiration_ = Timestamp();
    }
}