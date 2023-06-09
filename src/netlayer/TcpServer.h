#pragma once

#include "EventLoop.h"
#include "noncopyable.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "Callback.h"
#include "TcpConnection.h"

#include <functional>
#include <atomic>
#include <unordered_map>
#include <string>

// 上层使用核心组件 对外api
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    TcpServer(EventLoop *loop, 
    const InetAddress &listenAddr, const std::string &nameArg);
    ~TcpServer();

    // 设置底层subloop个数
    void setThreadNum(int numThreads);
    
    // 开启服务器监听
    void start();

    //设置Acceptor的new conncallback
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

private:
    //tcp connection
    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // base loop
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    // 新连接回调
    ConnectionCallback connectionCallback_;
    // 读写消息回调
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;

    //
    std::atomic<int> started_;
    //
    int nextConnId_;
    ConnectionMap connections_;
};
