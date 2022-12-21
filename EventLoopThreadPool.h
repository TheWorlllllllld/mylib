#pragma once 

#include "noncopyable.h"

#include <vector>
#include <functional>
#include <memory>
#include <string>

class EventLoopThread;
class EventLoop;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* loop,const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    //若为多线程，则mainloop默认以轮训方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoop();

    bool started() const { return started_; }
    const std::string& name() const { return name_; }
private:
    EventLoop* mainLoop_;
    std::string name_;
    bool started_;
    int numThreads_ = 0;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

