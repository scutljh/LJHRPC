#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

//此日志队列主要是为了保证我们的异步写在多线程情况下的并发安全

template<typename T>
class LogQueue
{
public:
    //可能有多个线程写（muduo）入队列
    void PushLog(const T& data)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_queue.push(data);
        log_cond.notify_one();
    }

    //一个线程读
    T PopLog()
    {
        std::unique_lock<std::mutex> lock(log_mutex);
        
        while(log_queue.empty())  //防止线程假唤醒
        {
            //则写入磁盘线程wait
            log_cond.wait(lock);
        }
        T data = log_queue.front();
        log_queue.pop();
        return data;
    }
private:
    std::queue<T> log_queue;
    std::mutex log_mutex;
    std::condition_variable log_cond;
};