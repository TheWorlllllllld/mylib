#pragma once 

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    bool listenning() const { return listening_; }
    void listen();
private:
    void handleRead();

    EventLoop *loop_;//mainloop
    Socket acceptSocket_;//accept socket
    Channel acceptChannel_;//accept channel
    NewConnectionCallback newConnectionCallback_;//新连接到来时的回调函数
    bool listening_;//是否正在监听
};
  