#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoop;

// one loop per thread的直观表现
// 将一个loop与thread进行绑定，让这个loop运行在此thread当中
class EventLoopThread : noncopyable
{
public:
    // 在当前线程中创建一个eventloop所执行的初始化回调
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    explicit EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                             const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    //线程要执行的回调
    void threadFunc();

    //核心成员
    EventLoop *loop_;
    Thread thread_;

    //辅助
    std::mutex mtx_;
    std::condition_variable cond_;

    ThreadInitCallback initCallback_;
    bool exiting_;
};
