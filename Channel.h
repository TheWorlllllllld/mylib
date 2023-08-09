#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

//相当于通道，里面包含了fd，注册的感兴趣的事件和内核返回的具体事件（EPOLLIN,EPOLLOUT）
class Channel:noncopyable
{
public:
    using EventCallback = std::function<void()>;//事件回调函数
    using ReadEventCallback = std::function<void(Timestamp)>;//读事件回调函数
    
    Channel(EventLoop* loop, int fd);
    ~Channel();

    //处理事件,根据epoll_wait返回的事件类型来调用对应的回调函数
    void handleEvent(Timestamp receiveTime);

    //设置回调函数
    void setReadCallback(ReadEventCallback cb)
    { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb)
    { writeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb)
    { errorCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb)
    { closeCallback_ = std::move(cb); }

    /*
    C++11对智能指针的应用，把channel所属的TcpConnect用一个弱智能指针记录下来，因为channel所绑定的每一个回调函数
    都是属于这个TcpConnect对象中的，假如这个TcpConnect被析构了，channel再去调用这些回调，那后果将不可想想，
    所以这里用一个弱智能指针来记录下来他，每次执行回调前都会检查这个TcpConnect还在不在，不在了就不执行回调了，
    绑定一个对象，当对象析构时，关闭通道
    */
    void tie(const std::shared_ptr<void>&);

    //获取fd
    int fd() const { return fd_; }
    //获取事件
    int events() const { return events_; }
    //设置事件
    void set_revents(int revts) { revents_ = revts; }
    //查看是否注册了事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    //向epoll中更改该fd对应的感兴趣事件,同时，注册事件也兼顾了往epoll中注册的事件
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    //查看是否还没注册感兴趣的事件
    bool idNoneEvent() const { return events_ == kNoneEvent; }
    //查看是否对写感兴趣
    bool isWriting() const { return events_ & kWriteEvent; }
    //查看是否对读感兴趣
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    //loop
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;//无事件
    static const int kReadEvent;//读事件
    static const int kWriteEvent;//写事件

    EventLoop *loop_;//所属的EventLoop
    const int fd_;//fd
    int events_;//感兴趣的事件
    int revents_;//内核返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;//是否绑定了一个对象

    // 通过从fd中获取的最终发生的具体时间revents，来调用具体的回调函数。
    ReadEventCallback readCallback_;//读事件回调函数
    EventCallback writeCallback_;//写事件回调函数
    EventCallback errorCallback_;//错误事件回调函数
    EventCallback closeCallback_;//关闭事件回调函数
    // EventCallback connnectedCallback_;//连接事件回调函数

};

