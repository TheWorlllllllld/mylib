#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb , const std::string& name)
                    :loop_(nullptr),
                    exiting_(false),
                    thread_(std::bind(&EventLoopThread::threadFunc,this),name),
                    mutex_(),
                    cond_(),
                    callback_(cb)
{
    // numCreated_ = 0;
}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){
    thread_.start();//启动线程
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

//该方法运行在单独的新线程里
void EventLoopThread::threadFunc(){
    EventLoop loop;  //创建一个EventLoop对象,实现 one loop per thread

    if(callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}