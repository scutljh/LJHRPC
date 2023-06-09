#pragma once
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

// socket地址封装

class InetAddress
{
public:
    // 两种构造
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);

    // accessor
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
    const sockaddr_in *getSockAddr() const { return &addr_; }

private:
    sockaddr_in addr_;
};