#include "TcpServer.h"
#include "Logger.h"
#include "InetAddress.h"
#include "TcpConnection.h"

#include <strings.h>

static EventLoop *CHECK_NOTNULL(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d main loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg)
    : loop_(CHECK_NOTNULL(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr))
    , threadPool_(new EventLoopThreadPool(loop, nameArg))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0)
{
    // tcpserver给acceptor设置  完成的事情就是connfd封装为channel，然后getnextloop到subloop并唤醒
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}
// 设置底层subloop个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::TcpServer::start()
{
    // 原子started_，防止多次started
    if (started_++ == 0)
    {
        // 启动线程池
        threadPool_->start(threadInitCallback_);
        // 开始监听连接
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// only mainloop do
// 老朋友，除了熟悉的功能外，还要设置消息读写的回调，方便subloop事件发生时使用
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *subloop = threadPool_->getNextLoop();

    // 封装connection  {channel
    char name_buf[64] = {0};
    snprintf(name_buf, 64, "-%s#%d", ipPort_.c_str(), nextConnId_++);
    std::string connName = name_ + name_buf;
    LOG_INFO("A newConnection [%s]\n", connName.c_str());

    // 通过sockfd获取其本机对应ip和端口
    sockaddr_in local_addr;
    socklen_t addrlen = sizeof local_addr;

    bzero(&local_addr, addrlen);
    if (getsockname(sockfd, (sockaddr *)&local_addr, &addrlen) < 0)
    {
        LOG_ERROR("getsockname error\n");
    }
    InetAddress localAddr(local_addr);

    //fd连接成功，包装tcpconnection对象
    TcpConnectionPtr conn(new TcpConnection(subloop,connName,sockfd,localAddr,peerAddr));
    
    //存储连接到server上
    connections_[connName] = conn;

    //vatal : user->tcpserver->tcpconnection->channel->poller->(event occured-notify)->channel=>callback
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);

    conn->setWriteCompleteCallback(writeCompleteCallback_);
    //设置关闭连接相关动作回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    
    //此时connection直接调用connectEstablished
    subloop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("remove tcp connection name %s \n",conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop* subloop = conn->getLoop();

    subloop->queueInLoop(std::bind(&TcpConnection::connectDestroy,conn));
}

TcpServer::~TcpServer()
{
    for(auto& elem:connections_)
    {
        TcpConnectionPtr tmp_conn(elem.second);
        elem.second.reset();  //释放智能指针之指向的对象 item不再指向，只有tmp_conn还在指向
        //其实不是多线程环境下就相当于:conn->connectDestroy  为了尽可能充分利用资源
        //当然一些简单的线程安全的accessor直接调用我觉得更好,这里getloop就是直接调用
        tmp_conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroy,tmp_conn)); 
    }
}