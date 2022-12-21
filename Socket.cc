#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

Socket::~Socket(){
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress& addr){
    if(::bind(sockfd_, (sockaddr*)addr.getSockAddr(), sizeof(sockaddr_in)) < 0){
        LOG_FATAL("Socket::bindAddress() failed ");
    }
}

void Socket::listen(){
    if(::listen(sockfd_, SOMAXCONN) < 0){
        LOG_FATAL("Socket::listen() failed ");
    }
}

int Socket::accept(InetAddress* peeraddr){
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    bzero(&addr, sizeof addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite(){
    if(::shutdown(sockfd_, SHUT_WR) < 0){
        LOG_FATAL("Socket::shutdownWrite() failed ");
    }
}

void Socket::setTcpNoDelay(bool on){
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval) < 0){
        LOG_FATAL("Socket::setTcpNoDelay() failed ");
    }
}

void Socket::setReuseAddr(bool on){
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0){
        LOG_FATAL("Socket::setReuseAddr() failed ");
    }
}

void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval) < 0){
        LOG_FATAL("Socket::setKeepAlive() failed ");
    }
}

void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    if(::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) < 0){
        LOG_FATAL("Socket::setReusePort() failed ");
    }
}