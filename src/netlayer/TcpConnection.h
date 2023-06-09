#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callback.h"
#include "Buffer.h"
#include "TimeStamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

//  打包成功连接服务器的客户端的通信链路
//  accept拿到connfd，打包tcpconnection  设置回调给channel  channel注册到poller上，从而调用回调
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    // accessor
    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    // callback setter

    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }

    // 连接的建立与销毁
    void connectEstablished();
    void connectDestroy();

    void send(const std::string& buf);
    void shutdown();

    bool connected() const { return state_ == Connected; }

private:
    enum StateE
    {
        Connecting,
        Connected,
        DisConnecting,
        DisConnected,
    };
    void setState(int state) { state_ = state; }

    void handleRead(TimeStamp recieveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    
    void sendInLoop(const std::string& buf);
    void shutdownInLoop();

    // subloop  对应一个tcpconnection
    EventLoop *loop_;
    const std::string name_;
    std::atomic<int> state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    // 新连接回调    user -- tcpserver -- tcpconnection -- channel
    ConnectionCallback connectionCallback_;
    // 读写消息回调  由用户设置
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
