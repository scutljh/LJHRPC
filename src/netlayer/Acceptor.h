#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class InetAddress;
class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop* loop,const InetAddress& listenAddr);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& newConnCB) 
    {newConnCB_=newConnCB;}
    
    bool isListenning() const {return listenning_;}
    void listen();
private:
    void handleRead();
    //需要一个事件循环   其实就是baseloop/mainloop
    EventLoop* loop_;
    //需要一个listenfd
    Socket acceptSocket_;
    //需要监听listenfd上的读写事件
    Channel acceptChannle_;

    //此回调主要负责连接成功后，选择subloop（getnextloop轮询选择）
    //，然后把对应连接成功fd打包发送到subloop中......
    NewConnectionCallback newConnCB_;
    bool listenning_;
};
