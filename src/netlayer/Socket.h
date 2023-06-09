#pragma once
#include "noncopyable.h"

class InetAddress;
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    ~Socket();

    //classic
    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress* peerAddr);

    void setKeepAlive(bool);
    void shutdownWrite();

private:
    const int sockfd_;
};
