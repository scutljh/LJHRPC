#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

//拷贝构造被delete了   用列表初始化
std::atomic<int> Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , name_(name)
    , func_(std::move(func))
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_&&!joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);
    
    //使用c++11 lambda表达式代替麻烦的结构体打包，轻松访问变量
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        //V原语唤醒
        sem_post(&sem);
        func_();
    }));
    //等待新创建的tid值被填入
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    // 用线程数代表线程序号作为名字
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, 32, "Thread%d", num);
        name_ = buf;
    }
}