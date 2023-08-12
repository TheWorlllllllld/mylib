#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"
#include "Callback.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

// TcpClient
class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg);
    ~TcpClient(); 

    void connect();
    void disconnect();
    void stop();

    // 用来获取可操控的连接对象TcpConnection
    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    EventLoop *getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    const std::string &name() const { return name_; }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        onConnection_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        onMessage_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        onWriteComplete_ = cb;
    }
private:
    void newConnection(int sockfd);

    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_; // avoid revealing Connector
    const std::string name_;
    ConnectionCallback onConnection_;
    MessageCallback onMessage_;
    WriteCompleteCallback onWriteComplete_;
    bool retry_;   // atomic   //是否重连，是指建立的连接成功后又断开是否重连。而Connector的重连是一直不成功是否重试的意思
    bool connect_; // atomic
    // always in loop thread
    int nextConnId_; // name_+nextConnid_用于标识一个连接
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_; //Connector连接成功后，得到一个TcpConnection
};