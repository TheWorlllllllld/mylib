#pragma once

#include <stdint.h>
#include <atomic>

#include "Timestamp.h"
#include "Callback.h"

// Timer类是对定时器的封装，包含了定时器的超时时间、
// 定时器的回调函数、定时器的重复时间间隔等信息
class Timer
{
public: 
    Timer(const TimerCallback &cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(numCreated_.fetch_add(1))
    {
    }

    void run() const
    {
        callback_();
    }

    // 获取定时器超时时间
    Timestamp expiration() const
    {
        return expiration_;
    }

    // 判断定时器是否重复
    bool repeat() const
    {
        return repeat_;
    }

    // 获取定时器序号
    int64_t sequence() const
    {
        return sequence_;
    }

    void restart(Timestamp now);

    // 获取定时器个数
    static int64_t numCreated()
    {
        return numCreated_;
    }

private:
    const TimerCallback callback_; // 定时器回调函数
    Timestamp expiration_; // 定时器超时时间
    const double interval_; // 定时器重复时间间隔
    const bool repeat_; // 是否重复
    const int64_t sequence_; // 定时器序号

    static std::atomic<int64_t> numCreated_; // 定时器计数器
};