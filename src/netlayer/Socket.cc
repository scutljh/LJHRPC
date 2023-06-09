#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <sys/socket.h>

Socket::~Socket()
{
    close(sockfd_);
}

// classic
void Socket::bindAddress(const InetAddress &localAddr)
{
    if (::bind(sockfd_, (sockaddr *)(localAddr.getSockAddr()), sizeof(sockaddr_in)) != 0)
    {
        LOG_FATAL("bind sockfd:%d failed \n", sockfd_);
    }
}

void Socket::listen()
{
    if (::listen(sockfd_, 1024) != 0)
    {
        LOG_FATAL("listen sockfd:%d failed \n", sockfd_);
    }
}

int Socket::accept(InetAddress *peerAddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, sizeof(addr));
    // 返回fd设置非阻塞
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (connfd >= 0)
    {
        peerAddr->setSockAddr(addr);
    }
    return connfd;
}

// to finish...
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof optval));
}

// EPOLLHUP
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shot down write error\n");
    }
}
