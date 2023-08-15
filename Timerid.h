#pragma once

#include <stdint.h>

class Timer;

class TimerId 
{
public:
    TimerId()
        : timer_(nullptr),
          sequence_(0)
    {
    }

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer),
          sequence_(seq)
    {
    }

    friend class TimerQueue;

private:
    Timer* timer_; //所属的定时器
    int64_t sequence_; //定时器序号
};