#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <atomic>
#include <memory>

static EventLoop *CHECK_NOTNULL(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d sub loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

// 很像acceptor，因为它们都是做一类事   一个tcpconnection对应一个subloop
TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(name), state_(Connecting), reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64mb
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection %s has created at fd %d\n ", name.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection %s destroyed at fd %d\n", name_.c_str(), socket_->fd());
}

void TcpConnection::handleRead(TimeStamp recieveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd(), &saveErrno);

    if (n > 0)
    {
        // 可读事件发生  调用用户回调
        messageCallback_(shared_from_this(), &inputBuffer_, recieveTime);
    }
    else if (n == 0) // 对端挂断
    {
        handleClose();
    }
    else // 连接异常
    {
        errno = saveErrno;
        LOG_ERROR("Tcpconnection handleRead error\n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) // 全发送到fd上了
            {
                channel_->disableWrite(); // 不必一直关注写，写往往就绪的，但用户不一定是就绪的
                if (writeCompleteCallback_)
                {
                    // loop_->queueInLoop(std::bind(writeCompleteCallback_,))
                    writeCompleteCallback_(shared_from_this());
                }

                if (state_ == DisConnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR(" TcpConnection::handleWrite error\n");
            handleError();
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d do not write \n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd = %d state=%d \n", channel_->fd(), static_cast<int>(state_));

    setState(DisConnected);
    channel_->disableAll();

    TcpConnectionPtr connSelf(shared_from_this());
    // onConnection本身就需要完成的
    if (connectionCallback_)
    {
        connectionCallback_(connSelf);
    }
    // 用户注册的
    if (closeCallback_)
    {
        closeCallback_(connSelf);
    }
}

// 先获取socketerror
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError err:%d\n", err);
}

// 接收到数据给客户端返回数据  //外层封装
void TcpConnection::send(const std::string &buf)
{
    if (state_ == Connected)
    {
        // 以相同的loop再返回数据
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf);
        }
        else
        {
            // debug
            loop_->queueInLoop(std::bind(
                &TcpConnection::sendInLoop, this, buf.c_str()));
        }
    }
}
/*
接收太快，但内核发的太慢，待发送需要全部写到缓冲区中
有水位回调的存在，保证数据不会丢失   debug
*/
// param:void*?     //实际内核
void TcpConnection::sendInLoop(const std::string &message) // vatal
{
    size_t msg_len = message.size();
    ssize_t writen = 0;
    size_t remaining = msg_len;
    bool saveError = false;

    if (state_ == DisConnected)
    {
        LOG_ERROR("Function:%s conn has disconnected,bad write\n", __FUNCTION__);
        return;
    }
    // 需要写时才关注写事件  并且  发送缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        writen = ::write(channel_->fd(), message.c_str(), msg_len);
        if (writen >= 0)
        {
            remaining = msg_len - writen;
            if (remaining == 0 && writeCompleteCallback_) // 一次性发送完毕  无需关注epollout事件
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // writen < 0
        {
            writen = 0;
            if (errno != EWOULDBLOCK) // 真出错了
            {
                LOG_ERROR("sendInLoop error!\n");
                if (errno == EPIPE || errno == ECONNRESET) // 发送错误了，对端链接重置
                {
                    saveError = true;
                }
            }
        }

        // 一次读超限制了，还有数据需要写入fd，将其保存到缓冲区中，然后channel关注写事件
        if (!saveError && remaining > 0)
        {
            // 缓冲区还残留一些待发送数据
            size_t oldlen = outputBuffer_.readableBytes();
            if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ && highWaterMarkCallback_)
            {
                loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
            }
            // 追加
            outputBuffer_.append(message.c_str() + writen, remaining); // debug

            if (!channel_->isWriting())
            {
                // 数据现在在缓冲区中，必须关注epollout以驱动回调
                channel_->enableWrite();
            }
        }
    }
}

//channel与tcpconnection的绑定，默认关注读事件
void TcpConnection::connectEstablished()
{
    setState(Connected);
    channel_->tie(shared_from_this());   //channel执行的回调都是tcpconnection给他的，因此必须确认tcpconnection是否存在，提升成功才执行相应回调
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroy()
{
    if(state_ == Connected)
    {
        setState(DisConnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this()); //classic 连接上和断开连接
    }
    channel_->remove();
}

// 让epoller感知EPOLLHUP从而执行closecallback
void TcpConnection::shutdown()
{
    if(state_ == Connected)
    {
        setState(DisConnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())  //说明发送缓冲区的数据已经全部发完
    {
        socket_->shutdownWrite();
    }

}
