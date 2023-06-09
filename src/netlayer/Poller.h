#pragma once

#include "noncopyable.h"
#include "TimeStamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// 实际监听 -- 封装epoll
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannel) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    //eventloop通过此接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    //此poller所关注的Channel  每个fd对应一个channel
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels;

private:
    EventLoop *ownerLoop_;  //每个poller有自己唯一的loop
};