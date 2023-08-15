#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "Timestamp.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <memory>

// 防止一个线程创建对个loop
__thread EventLoop* t_loopInThisThread = nullptr;

//默认超时时间
const int kPollTimeMs = 20000;

//创建wakefd，用来唤醒subReactor处理新来的channel
int creatEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("event error : %s  ", strerror(errno));
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,timerQueue_(new TimerQueue(this))
    ,wakeupFd_(creatEventfd())
    ,wakeupChannel_(new Channel(this, wakeupFd_))
    // ,currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d ", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d ", t_loopInThisThread, threadId_);
    }
    else{
        t_loopInThisThread = this;
    }
    //设置weakeupChannel的事件类型及其回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个subReactor都监听EPOLLIN，这样的话mainReactor就可以通过向wakeupChannel_发送信号来唤醒subReactor
    wakeupChannel_->enableReading(); //同时也是通过注册其关心事件来将其注册到epoll上。
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 ", n);
    }
}

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping ", this);

    while(!quit_){
        //调用Poller中的poll方法，返回值为活跃的Channel数量
        activeChannels_.clear();
        currentActiveChannel_ = nullptr;
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        //遍历活跃的Channel列表，调用Channel的handleEvent方法
        for(auto channel : activeChannels_){
            //poller将监听到的发生的事件上报给loop，loop将通知channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        //执行mainloop要求当前loop需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping ", this);
    looping_ = false;
}

void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()){ //如果是其他线程调用的quit
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    //唤醒需要执行上面回调函数的loop
    /*如果当前loop正在执行回调函数，但是loop又有了新的回调加入队列，他执行完老的回调之后势必会阻塞在poller_->poll函数上,
    *那么就需要用wake唤醒loop
    */ 
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

// 在指定的时间运行回调函数
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb){
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

// 运行回调函数，回调函数在delay时间后运行
TimerId EventLoop::runAfter(double delay, TimerCallback cb){
    Timestamp time(addTimer(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

// 运行回调函数，回调函数每隔interval时间运行一次
TimerId EventLoop::runEvery(double interval, TimerCallback cb){
    Timestamp time(addTimer(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

//取消定时器
void EventLoop::cancel(TimerId timerId){
    return timerQueue_->cancel(timerId);
}

//唤醒loop所在的线程的
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 ", n);
    }
}

//调用Poller中的方法
void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor : functors){
        functor();
    }
    callingPendingFunctors_ = false;
}


