#pragma once

#include "Timestamp.h"
#include "Callback.h"
#include "Channel.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <set>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

    int timerlen() const { return timers_.size(); }
    int activeTimerlen() const { return activeTimers_.size(); }
    int cancelingTimerlen() const { return cancelingTimers_.size(); }

private:
    using Entry = std::pair<Timestamp, Timer*>; // 定时器的到期时间和定时器
    using TimerList = std::set<Entry>; // 按照到期时间排序的定时器集合
    using ActiveTimer = std::pair<Timer*, int64_t>; // 定时器和序列号
    using ActiveTimerSet = std::set<ActiveTimer>; // 按照对象地址排序的定时器集合

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);

    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_; // 所属的EventLoop
    const int timerfd_; // timerfd
    Channel timerfdChannel_; // timerfd对应的Channel
    TimerList timers_; // 所有的定时器,按照到期时间排序

    ActiveTimerSet activeTimers_; // 所有的活动定时器,按照对象地址排序
    bool callingExpiredTimers_; // 是否正在调用回调函数
    ActiveTimerSet cancelingTimers_; // 所有的取消的定时器
};