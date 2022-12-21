#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Logger.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* mainloop,const std::string& nameArg)
    :mainLoop_(mainloop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{    
}

EventLoopThreadPool::~EventLoopThreadPool(){

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb){
    started_ = true;

    for(int i = 0;i < numThreads_;i++){
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        EventLoopThread* t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    if(numThreads_ == 0 && cb){
        cb(mainLoop_);
    }
}

//若为多线程，则mainloop默认以轮训方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop *loop_ = mainLoop_;
    if(!loops_.empty()){
        loop_ = loops_[next_];
        ++next_;
        if(next_ >= numThreads_){
            next_ = 0;
        }
    }
    return loop_;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoop(){
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,mainLoop_);
    }
    else{
        return loops_;
    }
}