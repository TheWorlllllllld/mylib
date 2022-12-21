#include "InetAddress.h"

#include <string.h>


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    return std::string(inet_ntoa(addr_.sin_addr));
}

std::string InetAddress::toIpPort() const
{
    char buf[32] = {0};
    snprintf(buf, sizeof buf, "%s:%d", toIp().c_str(), toPort());
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}