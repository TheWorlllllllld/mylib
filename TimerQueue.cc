#include "EventLoop.h"
#include "Logger.h"
#include "TimerQueue.h"
#include "Timer.h"
#include "Timerid.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <functional>
#include <vector>
#include <assert.h>

namespace detail
{
    // 用于创建读timerfd的fd
    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if(timerfd < 0)
        {
            LOG_FATAL("Failed in timerfd_create");
        }
        return timerfd;
    }

    // 用于计算超时时间
    struct timespec howMuchTimeFromNow(Timestamp when)
    {
        int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
        if(microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    // 用于读timerfd
    void readTimerfd(int timerfd, Timestamp now)
    {
        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
        // LOG_TRACE("TimerQueue::handleRead() {} at {}", howmany, now.toString());
        if(n != sizeof(howmany))
        {
            LOG_ERROR("TimerQueue::handleRead() reads {} bytes instead of 8");
        }
    }

    // 用于重置timerfd
    void resetTimerfd(int timerfd, Timestamp expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        memset(&newValue, 0, sizeof(newValue));
        memset(&oldValue, 0, sizeof(oldValue));
        newValue.it_value = howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if(ret)
        {
            LOG_ERROR("timerfd_settime()");
        }
    }
}

using namespace detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

// 用于添加定时器
/*
TimerCallback：定时器回调函数
Timestamp：定时器超时时间
double：定时器超时后的重复间隔，如果为0，则不重复
*/  
TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

// 用于取消定时器
void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

// 用于添加定时器
void TimerQueue::addTimerInLoop(Timer* timer)
{
    bool earliestChanged = insert(timer);
    // 如果最早的定时器发生了改变，则重置timerfd
    if(earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 用于取消定时器
void TimerQueue::cancelInLoop(TimerId timerId)
{
    // 如果定时器正在执行回调函数，则延迟取消
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    auto it = activeTimers_.find(timer);
    /*
    正在被执行回调的定时器不会在activeTimers_中，
    所以如果在activeTimers_中找到了该定时器，则说明该定时器不在执行回调
    再判断下是否在执行回调，是为了防止在执行回调的过程中，该定时器被取消
    若在执行回调的过程中，该定时器被取消，则会在cancelingTimers_中找到该定时器
    */
    if(it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        delete it->first;
        activeTimers_.erase(it);
    }
    else if(callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

// 用于处理定时器超时
void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    // 获取超时的定时器
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    // 执行超时定时器的回调函数
    for(const Entry& it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;

    // 重置重复定时器，并将重复定时器插入到timers_中，
    // 即重复执行的定时器
    reset(expired, now);
}

// 用于获取超时的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    // 用于查找第一个未超时的定时器
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    // 返回第一个未超时的定时器的迭代器
    auto end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    // 将超时的定时器插入到expired中
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    // 从activeTimers_中删除超时的定时器
    for(const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
    }
    assert(timers_.size() == activeTimers_.size());
    return expired;
}

// 用于重置重复定时器
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    // 用于记录下一个超时的定时器
    Timestamp nextExpire;
    for(const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        // 如果定时器是重复的，则重置定时器
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }
    // 如果timers_不为空，则重置timerfd
    if(!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }
    // 如果nextExpire不为空，则重置timerfd
    if(nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}

// 用于插入定时器
bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto it = timers_.begin();
    // 如果timers_为空或者when小于timers_中的最早的定时器，则earliestChanged为true
    if(it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}