#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

// 管理eventloop thread
//thread --> eventloop thread --> eventloop thread poll

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) {numThreads_ = numThreads;}

    //根据指定threads初始化线程池
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    //baseloop的负载均衡  基础的轮询方式分配channel
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();
    
    //some accossor
    bool started() const {return started_;}
    const std::string name() const {return name_;}

private:
    EventLoop *baseLoop_; // 至少得有一个loop  mainloop
    
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;

    //核心存储
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};