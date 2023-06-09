#include "Channel.h"
#include "TimeStamp.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::NoneEvent_ = 0;
const int Channel::ReadEvent_ = EPOLLIN | EPOLLPRI;
const int Channel::WriteEvent_ = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

//Channel的tie方法 在一个tcpconnection创建的时候与其绑定，weakptr
void Channel::tie(const std::shared_ptr<void> &sp)
{
    tie_ = sp;
    tied_ = true;
}

//去到poller中改变，真正在做事
void Channel::update()
{
    //通过回指的loop找到此Channel对应的loop从而拿到poller
    loop_->updateChannel(this);
}

//在此channel对应的loop中，删除当前channel
void Channel::remove()
{
    loop_->removeChannel(this);
}

//fd关注的事件发生后要做的    tie_触发时机影响此处逻辑
void Channel::handleEvent(TimeStamp receiveTime)
{
    //保证不会调用一个已经销毁了的对象
    if(tied_)
    {
        std::shared_ptr<void> guard;
        guard = tie_.lock();
        handleEventWithGuard(receiveTime);
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

//guard封装，真实调用
void Channel::handleEventWithGuard(TimeStamp receiveTime)
{
    LOG_INFO("channel handle revents:%d\n",revents_);

    if((revents_&EPOLLHUP)&&!(revents_&EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_&EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    if(revents_&(EPOLLIN|EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if(revents_&(EPOLLOUT))
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}

