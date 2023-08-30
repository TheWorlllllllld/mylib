#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callback.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <string>
#include <memory>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable , public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return local_addr_; }
    const InetAddress& peerAddress() const { return peer_addr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buf);

    void shutdown();

    void setHighWaterMarkCallback(HighWaterMarkCallback cb)
    { highWaterMarkCallback_ = cb; }
    void setConnectionCallback(ConnectionCallback cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(MessageCallback cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    { writeCompleteCallback_ = cb; }
    void setCloseCallback(closeCallback cb)
    { closeCallback_ = cb; }

    //连接建立
    void connectEstablished();
    //连接销毁
    void connectDestroyed();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();

private:
    enum StateE{
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected,
        kClosing,
        kClosed,
    };

    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    EventLoop *loop_; //首先排除mainloop，TcpConnection是用来和subloop建立通信的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress local_addr_;//本机
    const InetAddress peer_addr_;//对端

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    //写入完成的事件回调
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    closeCallback closeCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
