#pragma once 

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

//多路复用核心，epoll主体
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    
    Poller(EventLoop* loop);
    virtual ~Poller();

    //给所有io复用保留接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    //判断channel是否在poller中
    bool hasChannel(Channel* channel) const;

    //获取poller的类型
    static Poller* newDefaultPoller(EventLoop* loop);


protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    //key:fd,value:channel
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;//当前poller所属的loop
};