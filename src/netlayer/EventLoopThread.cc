
#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &initcb,
                                 const std::string &name)
    : loop_(nullptr)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , exiting_(false)
    , mtx_()
    , cond_()
    , initCallback_(initcb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

// 开启一个loop，返回成功开启的loop的指针
EventLoop *EventLoopThread::startLoop()  //debug 只发生了一次  fixed
{
    thread_.start(); // 底层就是开启新的线程，执行threadFunc

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        while (loop_ == nullptr)   //这里写错了，loop_和loop  debug了一年
        {
            cond_.wait(lock);
        }   //debug  循环没出来
        // loop就绪
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()  //debug  只执行了一次？  fixed
{
    EventLoop loop; // 启动线程后，才真正创建全新独立的eventloop与此线程绑定

    if (initCallback_)
    {
        initCallback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mtx_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop => Poller poll //一般不会再下来

    std::unique_lock<std::mutex> lock(mtx_);
    loop_ = nullptr;
}