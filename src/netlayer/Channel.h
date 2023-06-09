#pragma once

#include "noncopyable.h"
#include "TimeStamp.h"

#include <functional>
#include <memory>

// 头文件前置声明，源文件中再加相关头文件，对外暴露信息更少
class EventLoop;
// class TimeStamp;

// EventLoop的核心组件之一：Channel
// Channel通道，其实就是对fd和其感兴趣的事件以及所关注的事件中发生的事件的封装
class Channel : noncopyable
{
public:
    // 声明两种回调
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // channel核心：事件发生，处理事件
    void handleEvent(TimeStamp receiveTime); // 和evtloop指针不同，指针大小编译可确定，次不可，因此前置声明不起作用

    // 设置事件回调
    void setReadCallback(ReadEventCallback rcb) { readCallback_ = std::move(rcb); }
    void setWriteCallback(EventCallback ecb) { writeCallback_ = std::move(ecb); }
    void setCloseCallback(EventCallback ecb) { closeCallback_ = std::move(ecb); }
    void setErrorCallback(EventCallback ecb) { errorCallback_ = std::move(ecb); }

    void tie(const std::shared_ptr<void> &);

    // some accessor
    int fd() const { return fd_; }
    int events() const { return events_; }
    // 给poller提供的
    int set_revents(int revents) { revents_ = revents; }

    // 使能接口  关于fd  就是一层封装，和epoll_ctl相关
    void enableReading()
    {
        events_ |= ReadEvent_;
        update();
    }
    void disableReading()
    {
        events_ &= ~ReadEvent_;
        update();
    }
    void enableWrite()
    {
        events_ |= WriteEvent_;
        update();
    }
    void disableWrite()
    {
        events_ &= ~WriteEvent_;
        update();
    }
    void disableAll()
    {
        events_ = NoneEvent_;
        update();
    }

    //fd事件状态
    bool isReading() const { return events_ & ReadEvent_; }
    bool isWriting() const { return events_ & WriteEvent_; }
    bool isNoneEvent() const { return events_ == NoneEvent_; }

    //for poller
    int index() {return index_;}
    void set_index(int index) {index_ = index;}

    //回指定位
    EventLoop* ownerLoop() {return loop_;}

    //删除channel
    void remove();

private:

    void update();
    //核心  handleEvent
    void handleEventWithGuard(TimeStamp receiveTime);

    // 事件标志位  EPOLLIN/OUT...
    static const int NoneEvent_;
    static const int ReadEvent_;
    static const int WriteEvent_;

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;        //类似宏NEW_，表示此channel在poller中的状态 叫status更为合适

    std::weak_ptr<void> tie_;
    // weak_ptr的第二个作用：防止此channel对象被remove掉以后，还有别的地方在使用对象。
    // 多线程环境下，跨线程的一个对象生存状态监听器，使用的时候可以把weakptr提升为强的智能指针
    // 提升失败就说明资源已释放，别访问就行了  --  也可以使用shared_from_this
    bool tied_;

    // channel直接知道fd要发生的事件，然后对具体事件调用回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};