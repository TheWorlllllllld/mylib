#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

const int KNew = -1;//还没添加
const int KAdded = 1;// 已经添加
const int KDeleted = 2;// 已经删除

EPollPoller::EPollPoller(EventLoop *loop): Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ == -1)
    {
        LOG_FATAL("epoll_creat error: %d %s ", errno, strerror(errno));
        abort();
    }
}

EPollPoller::~EPollPoller(){
    close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels){
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0){
        
        // LOG_INFO("epoll events: %d ", numEvents);

        fillActiveChannels(numEvents, activeChannels);
        if(static_cast<size_t>(numEvents) == events_.size()){
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0){
        LOG_DEBUG("%s poll timeout ", _FUNCTION_);
    }
    else{
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll error: %d %s ", errno, strerror(errno));
        }
    }
    return now;
}

//调用顺序：chanell->loop->poller->updateChannel
void EPollPoller::updateChannel(Channel *channel){
    const int index = channel->index();
    // LOG_INFO("fd = %d, events = %d, index = %d", channel->fd(), channel->events(), index);

    if(index == KNew || index == KDeleted){  //epoll上没有该channel，或者该channel已经被删除,说明是想被加进epoll上
        if(index == KNew){
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(KAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else{  //epoll上已经有该channel
        int fd = channel->fd();
        if(channel->isNoneEvent()){//该channel没有事件，说明是想被移除epoll上
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(KDeleted);
        }
        else{//该channel有事件，说明是想被更新epoll上
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//调用顺序：chanell->loop->poller->removeChannel
void EPollPoller::removeChannel(Channel *channel){
    int fd = channel->fd();
    int index = channel->index();
    channels_.erase(fd);
    if(index == KAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(KNew);
}

//填写活跃链接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels){
    for(int i = 0; i < numEvents; ++i){
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

//更新channel的感兴趣事件
void EPollPoller::update(int operation, Channel *channel){
    epoll_event event;
    int fd = channel->fd();
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.fd = fd; 
    event.data.ptr = channel;
    if(::epoll_ctl(epollfd_, operation, fd, &event) <0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl DEL error: %d %s ", errno, strerror(errno));
        }
        else{
            LOG_FATAL("epoll_ctl ADD/MOD error: %d %s ", errno, strerror(errno));
        }
    }
}