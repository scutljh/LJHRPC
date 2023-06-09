#pragma once
#include "noncopyable.h"
#include "CurrentThread.h"
#include "TimeStamp.h"

#include <functional>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>

class Channel;
class Poller;
// 事件循环  包含关键的Channel和Poller(我只封装了epoll模型)
//   一个eventLoop对应一个线程
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    TimeStamp pollReturnTime() const {return pollReturnTime_;}

    void runInLoop(Functor cb);
    //等待队列，放入唤醒其所在线程，然后执行相关回调
    void queueInLoop(Functor cb);
    void wakeup();
    //查看是否是此loop对应的线程，不是就进queue，是则执行
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

    //老朋友，调poller
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

private:
    using ChannelList = std::vector<Channel *>;

    // c++11原子操作 底层CAS
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    // 正在处理其他事件标志位   (pending表中的
    std::atomic<bool> callingPendingFunctors_;
    TimeStamp pollReturnTime_;   //poller返回发生事件的channel的时间点
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;

    //与eventfd系统调用强相关，mainloop选择并通知唤醒subloop的核心
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;

    //需要被执行的回调存放pending表
    std::vector<Functor> pendingFunctors_;
    mutable std::mutex mtx_;

    void handleRead();
    void doPendingFunctors();
};