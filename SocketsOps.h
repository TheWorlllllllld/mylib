#pragma once

#include <arpa/inet.h>

namespace sockets {
    //TODO server used api


    int getSocketError(int sockfd);
    
    struct sockaddr_in getLocalAddr(int sockfd);
    struct sockaddr_in getPeerAddr(int sockfd);
    bool isSelfConnect(int sockfd);
}