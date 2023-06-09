#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <unistd.h>
#include <cstring>

// channel在poller中的状态
const int NEW_ = -1;    // 未添加
const int ADDED_ = 1;   // 已添加
const int DELETED_ = 2; // 已删除

// 如此创建epfd是为了让  子进程被替换时会关闭打开的文件描述符（默认继承了）
EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), epfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(InitEventListSize)
{
    if (epfd_ < 0)
    {
        LOG_FATAL("can't create epoll fd");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epfd_);
}

/*
 *@brief epoll_wait
 *@param timeoutMs: i
 *@param activeChannels: o
 */
TimeStamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 防止影响效率,此接口中的所有日志应该使用LOG_DEBUG("")
    LOG_INFO("func=%s => fd counts :%d\n", __FUNCTION__, channels.size());

    // 因vector，但需要拿到数组首元素地址，如何做？
    // 使用迭代器拿到首元素位置，然后解引用拿到元素，再取地址   vector底层连续空间
    int active_events_num = ::epoll_wait(epfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);

    // 多线程访问可能修改全局变量errno，因此存储
    int saveErrno = errno;
    TimeStamp now(TimeStamp::now());

    if (active_events_num > 0)
    {
        LOG_INFO("%d events has occurred\n", active_events_num);
        fillActiveChannel(active_events_num, activeChannels);

        // 当发生事件数量等于了最大容量，及时扩容
        if (active_events_num == events_.size())
        {
            events_.resize(events_.size() * 1.5);
        }
    }
    else if (active_events_num == 0)
    {
        LOG_DEBUG("%s timeout ! \n", __FUNCTION__);
    }
    else  //some default
    {
        //由除了外部中断的其它错误引起
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error!");
        }
    }
    return now;
}

// epoll_ctl
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd = %d events = %d index = %d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == NEW_ || index == DELETED_)
    {
        if (index == NEW_) // 此channel从来没有被poller管理过
        {
            int fd = channel->fd();
            channels[fd] = channel;
        }
        channel->set_index(ADDED_);
        update(EPOLL_CTL_ADD, channel); // 触摸系统调用
    }
    else // 此channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(DELETED_);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
// 将某channel彻底从此poller中移除，oop层面不关注
void EpollPoller::removeChannel(Channel *channel)
{
    int index = channel->index();
    LOG_INFO("func=%s => fd = %d \n", __FUNCTION__, channel->fd());

    if (index == NEW_)
    {
        LOG_FATAL("Try to delete a channel that never be added into Poller");
    }
    if (index == ADDED_)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channels.erase(channel->fd());
    channel->set_index(NEW_);
}

void EpollPoller::fillActiveChannel(int NumEvents, ChannelList *activeChannels) const
{
    for(int i=0;i<NumEvents;++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 和epoll系统调用相关
void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = channel->events();
    event.data.ptr = channel; // data只需要填充这个指针  可以通过这个拿到其它信息
    int fd = channel->fd();

    if (::epoll_ctl(epfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll ctl op : delete failed");
        }
        else
        {
            LOG_FATAL("epoll ctl op : add or mod failed");
        }
    }
}