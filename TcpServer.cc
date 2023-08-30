#include "TcpServer.h"
#include "Logger.h"

#include <functional>
#include <strings.h>

EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d: mainloop is nullptr ", __FILE__, __FUNCTION__, __LINE__);
        return nullptr;
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string &name , Option option)
            : loop_(CheckLoopNotNull(loop))
            , ipPort_(listenAddr.toIpPort())
            , name_(name)
            , acceptor_(new Acceptor(loop_, listenAddr, option == kReusePort))
            , threadPool_(new EventLoopThreadPool(loop_, name_))
            , onConnection_()
            , onMessage_()
            , nextConnId_(1)
            , started_(0)
{
    //新用户连接时的回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer(){
    LOG_INFO("%s:%s:%d: TcpServer %s is destructing ", __FILE__, __FUNCTION__, __LINE__, name_.c_str());
    
    for(auto& conn : connections_){
        TcpConnectionPtr connPtr(conn.second);//改用临时变量connPtr来指向conn.second，方便释放
        conn.second.reset();
        connPtr->getLoop()->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, connPtr));
    }
}

void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(){
    if(started_++ == 0){
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

//当有新的连接产生，那acceptor会调用这个函数
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    //轮询选择一个subloop
    EventLoop* subloop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    // LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t len = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &len) < 0)
    {
        LOG_ERROR("TcpServer::getLocalAddr getsockname error");
        ::close(sockfd);
        return;
    }

    InetAddress localAddr(local);
    
    //根据新到来的连接，将其封装为一个TcpConnect对象
    TcpConnectionPtr conn(new TcpConnection(subloop, 
                                            connName, 
                                            sockfd, 
                                            localAddr, 
                                            peerAddr));
    //将这个连接放入连接map中
    connections_[connName] = conn;
    //调用连接的回调函数:Tcpserver -> TcpConnection -> Channel -> EventLoop -> notify ->channel
    conn->setConnectionCallback(onConnection_);
    conn->setMessageCallback(onMessage_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    conn->setWriteCompleteCallback(onWriteComplete_);
    //将连接加入到subloop中
    subloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
    // LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection [%s] ", name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop* subLoop = conn->getLoop();
    subLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
