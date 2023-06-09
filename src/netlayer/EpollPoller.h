#pragma once

#include "Poller.h"
#include "TimeStamp.h"

#include <sys/epoll.h>
#include <vector>

class Channel;

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    TimeStamp poll(int timeoutMs, ChannelList *activeChannels) override;

    //epoll_ctl
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int InitEventListSize = 16; 
    //填入活跃的fd--channel
    void fillActiveChannel(int NumEvents,ChannelList* activeChannels) const;
    //更新channel上的事件关注
    void update(int operation,Channel* channel);

    using EventList = std::vector<struct epoll_event>;
    int epfd_;
    EventList events_;   //发生事件的epoll_event集合
};