#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("MUDUO_USE_EPOLL"))
    {
        // return new EpollPoller(loop);
        return nullptr;
    }
    else
    {
        // return new SelectPoller(loop);
        return new EPollPoller(loop);
    }
}