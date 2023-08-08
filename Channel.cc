#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>
#include <assert.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

//将该Channel的所属loop标记下来
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false) 
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

//处理事件，根据epoll_wait返回的事件类型来调用对应的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // LOG_INFO("channel handleEvent revents:%d", revents_);

    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        // if(tied_){
        //     tied_ = false;
        // }
        if(closeCallback_){
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}

//改变channel感兴趣的事件后，用update来改变epoll里fd的监听事件
void Channel::update()
{
    //通过channel的loop来更改events
    loop_->updateChannel(this);
}

//通过channel的loop来删除channel
void Channel::remove()
{
    loop_->removeChannel(this);
}