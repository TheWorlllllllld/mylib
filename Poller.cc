#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop): ownerLoop_(loop)
{
}

Poller::~Poller() = default;

//判断channel是否在poller中
bool Poller::hasChannel(Channel* channel) const{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

