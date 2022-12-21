#pragma once   

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

//时间循环类，包含epoll，Channel等类，用于处理事件
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop();//开启事件循环
    void quit();//退出事件循环

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    //在当前loop中执行一个函数
    void runInLoop(Functor cb);
    //在当前loop中延迟执行一个函数
    void queueInLoop(Functor cb);
    
    //唤醒loop所在的线程的
    void wakeup();

    //调用Poller中的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    
    //判断当前线程是否是loop所在的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid();}
private:
    //唤醒loop的函数
    void handleRead();
    //执行回调函数
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;//退出loop循环

    const pid_t threadId_;//当前loop所在线程id

    Timestamp pollReturnTime_;//epoll_wait返回时间
    std::unique_ptr<Poller> poller_;//epoll实例

    int wakeupFd_;//唤醒线程的fd
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;//活跃的Channel列表
    Channel *currentActiveChannel_;//当前活跃的Channel

    std::atomic_bool callingPendingFunctors_;//是否有要执行的回调函数
    std::vector<Functor> pendingFunctors_;//要执行的回调函数列表
    std::mutex mutex_;//互斥锁 保护vector的线程安全
};