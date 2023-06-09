#include "InetAddress.h"

#include <string.h>
#include <strings.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_,sizeof(addr_));
    addr_.sin_family=AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}
InetAddress::InetAddress(const sockaddr_in &addr)
{
    addr_.sin_family = addr.sin_family;
    addr_.sin_port = addr.sin_port;
    addr_.sin_addr.s_addr = addr.sin_addr.s_addr;
}

// accessor
std::string InetAddress::toIp() const
{
    char ipbuf[64] = {0};
    //从addr读出ip地址的内容
    ::inet_ntop(AF_INET,&addr_.sin_addr,ipbuf,sizeof(ipbuf));
    return ipbuf;
}
std::string InetAddress::toIpPort() const
{
    //format  --  ip::port
    char ipbuf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,ipbuf,sizeof(ipbuf));

    size_t end = strlen(ipbuf);
    uint16_t port = ntohs(addr_.sin_port);

    sprintf(ipbuf+end,":%u",port);

    return ipbuf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}