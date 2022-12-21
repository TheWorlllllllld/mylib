#include "TcpConnection.h"
#include "Socket.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d: TcpConnection loop is nullptr", __FILE__, __FUNCTION__, __LINE__);
        return nullptr;
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
            : loop_(CheckLoopNotNull(loop)),
              name_(nameArg),
              state_(kConnecting),
              reading_(true),
              socket_(new Socket(sockfd)),
              channel_(new Channel(loop, sockfd)),
              local_addr_(localAddr),
              peer_addr_(peerAddr),
              highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor [%s] fd=%d ", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor [%s] fd=%d state=%d ", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf){
    if(state_ == kConnected){
        if(loop_ -> isInLoopThread()){//防止不在当前线程，比如某些场景将TcpConnection存起来的时候，不在当前线程
            sendInLoop(buf.c_str(), buf.size());
        }
        else{
            loop_ -> runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){ //只有当数据全部发完了才能真正去让poller去通知channel_该调用TcpConnection::handleClose()这个回调了
        socket_->shutdownWrite();//通过将与对端之间的连接断开，来让epoll监听到连接断开
    }
}

//发送数据,将数据写入缓冲区，并设置水位线
void TcpConnection::sendInLoop(const void* data, size_t len){
    ssize_t nwrote = 0;
    size_t remaining = len;//没发送完的数据
    bool faultError = false;

    if(state_ == kDisconnected){//说明调用过shutdown，不能在发送了
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    //表明缓冲区没有待发送的数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                //数据一次性发送完成了，那就不用再向epoll中注册EPOLLOUT事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else{ //<0
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)//接收到对端的断开
                {
                    faultError = true;
                }
            }
        }
    }
    //还有数据没发送完，那就把数据放到缓冲区里面,给这个channel向epoll中注册EPOLLOUT事件，等待poller检测到tcp内核区的缓冲区
    //有空余空间，那便会通知这个channel去写数据，既调用TcpConnection::handleWrite
    if(!faultError && remaining > 0){
        //目前buffer中剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            //第一个条件表明，马上要发送的数据大于等于了水位线，
            //第二个条件若大于表明原先buffer的数据本来就大于了水位线，说明早就执行了大于水位线回调函数
            loop_ -> queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if(!channel_->isWriting()){//若没注册写事件，那就注册进去
            channel_->enableWriting();
        }
    }
}

//连接建立
void TcpConnection::connectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    //执行新连接建立的回调
    connectionCallback_(shared_from_this());
}

//连接销毁
void TcpConnection::connectDestroyed(){
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}


void TcpConnection::handleRead(Timestamp receiveTime){
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0){//读取到数据
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0){//断开了连接
        handleClose();
    }
    else{
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead err: %s", strerror(errno));
        handleError();
    }
}

void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if(n > 0){
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){//如果发完了就去除掉注册的EPOLLOUT
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            if(state_ == kDisconnecting){
                shutdownInLoop();
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite ");
        }
    }
    else{
        LOG_ERROR("TcpConnection fd = %d is not writing", channel_->fd());
    }
}

void TcpConnection::handleClose(){
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d ", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);//连接断开也要执行这个回调来通知上层
    closeCallback_(connPtr);
}

void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        err = errno;
    }
    else{
        errno = optval;
    }
    LOG_ERROR("TcpConnection::handleError SO_ERROR:%s  ", strerror(err));
}


