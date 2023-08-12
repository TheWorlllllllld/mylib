#include "Connector.h"

#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "SocketsOps.h"

#include <string>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <assert.h>

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG("ctor[%p]", this);
}

Connector::~Connector()
{
    LOG_DEBUG("dtor[%p]", this);
    assert(!channel_);
}

// 发起连接
void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    assert(state_ == kDisconnected);
    // 必须为true，start()中会设置为true
    if (connect_) 
    {
        connect(); //连接的具体实现
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    // TODO 完善socket工具类
    // 创建非阻塞socket
    int sockfd = ::socket(serverAddr_.getSockAddr()->sin_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_ERROR("socket error[%d]", errno);
        return;
    }
    // TODO 完善socket工具类
    int ret = ::connect(sockfd, (sockaddr*)serverAddr_.getSockAddr(), sizeof(struct sockaddr_in));
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) // 错误码处理
    {
        case 0:
        case EINPROGRESS: // 非阻塞套接字，未连接成功返回码是EINPROGRESS表示正在连接
        case EINTR:
        case EISCONN: // 连接成功
            connecting(sockfd);
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd); // 重试
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR("connect error[%d]", savedErrno);
            ::close(sockfd); //这几种情况下，不会重试，直接关闭套接字
            break;
        default:
            LOG_ERROR("Unexpected error[%d]", savedErrno);
            ::close(sockfd);
            break;
    }
}

void Connector::restart()
{
    setState(kDisconnected);
    // 重试时间递增
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

// 若连接成功，则将临时channel注册进去，
// 这里的channel只管理建立连接阶段。连接建立后，交给TcoConnection管理
// 注册的handleWrite只是负责回调newConnectionCallback_即TcpClient::newConnection
// newConnection是为了将fd托管给TcpConnection
void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

// 若连接成功，执行回调newConnectionCallback_
void Connector::handleWrite()
{
    if (state_ == kConnecting)
    {
        // 从epoll中移除临时channel
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err){
            LOG_ERROR("Connector::handleWrite - SO_ERROR[%d]", err);
            retry(sockfd);
        } else if(sockets::isSelfConnect(sockfd)){ // 判断是否是自连接
            LOG_ERROR("Connector::handleWrite - Self connect");
            retry(sockfd);
        } else {
            setState(kConnected);
            if(connect_){
                newConnectionCallback_(sockfd);
            } else {
                ::close(sockfd);
            }
        }
    }
    else
    {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR("Connector::handleError state[%d]", state_);
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_ERROR("SO_ERROR[%d]", err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    ::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO("Connector::retry - Retry connecting to %s in %d milliseconds.", serverAddr_.toIpPort().c_str(), retryDelayMs_);
        // TODO 完善定时器
        // loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}