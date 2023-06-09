#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <atomic>
#include <string>

#include "noncopyable.h"


//封装的线程对象  -- 为了提供一种更方便的抽象和使用
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_; // c++11 thread创建就启动
    pid_t tid_;
    ThreadFunc func_;
    std::string name_; // debug

    static std::atomic<int> numCreated_;
};