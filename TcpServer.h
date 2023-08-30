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

//主体
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string &name , Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { onConnection_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { onMessage_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { onWriteComplete_ = cb; }

    void setThreadNum(int numThreads);

    void start();

protected:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    const std::string ipPort_;//ip:port
    const std::string name_;//server name

    EventLoop *loop_;//mainloop
    std::unique_ptr<Acceptor> acceptor_;//acceptor
    std::shared_ptr<EventLoopThreadPool> threadPool_;//thread pool

    //callback
    ConnectionCallback onConnection_;//有新连接的回调
    MessageCallback onMessage_;//有读写事件的回调
    WriteCompleteCallback onWriteComplete_;//消息发送完成的回调
    // closeCallback onClose_;//close callback

    //thread init callback
    ThreadInitCallback threadInitCallback_;//线程初始化的回调
    std::atomic_int started_;//是否已经启动

    int nextConnId_;//下一个连接id
    ConnectionMap connections_;//保存所有连接
};