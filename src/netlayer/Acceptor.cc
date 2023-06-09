#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <functional>
#include <unistd.h>

// 2.6.27版本以后非常方便，socket创建时即可让其非阻塞
// 我记得我以前使用的fcntl好像
static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK| SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d : create socket listen fd failed,err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}
// main reactor/loop 核心

// 非阻塞的listen sock
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop), acceptSocket_(createNonblocking()), acceptChannle_(loop, acceptSocket_.fd()), listenning_(false)
{
    acceptSocket_.bindAddress(listenAddr);
    acceptChannle_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannle_.enableReading();  //此channel在poller中需要关注读事件，查看是否有新连接，有就回调handleread
}

// 监听listen_fd是否有事件发生的channel，处理新用户到来的fd连接
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        // 做：打包fd为channel，getnextloop分发给subloop
        if (newConnCB_)
        {
            newConnCB_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d : accept fd failed,err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);

        // 或许是fd上限了
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d : sock fd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

Acceptor::~Acceptor()
{
    //channel从当前poller中清除掉 --> 事件不关注，删除
    acceptChannle_.disableAll();
    acceptChannle_.remove();
}
