#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>
#include <iostream>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{
}
EventLoopThreadPool::~EventLoopThreadPool()
{
}

// 根据指定threads初始化线程池   把每个线程都创建启动起来，执行loop->loop
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)  //循环仅进入了一次
    {
        char name_buf[name_.size() + 32];
        snprintf(name_buf, sizeof name_buf, "%s%d", name_.c_str(), i);
        //do things
        EventLoopThread *t = new EventLoopThread(cb, name_buf);
        // 下标也是一一对应
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
        //debug  无法执行到这
    }

    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// baseloop的负载均衡  基础的轮询方式分配channel 取下一个loop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;
    if (!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}