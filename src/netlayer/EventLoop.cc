#include "EventLoop.h"
#include "Logger.h"
#include "CurrentThread.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <errno.h>

// 全局控制，防止一个线程创建多个EventLoop，创建时会判是否为空，为空才允许创建
//__thread声明 就是相当于多个线程对于此全局变量有多个副本，每个线程控制自己的，类似thread_local

__thread EventLoop *t_loopInThisThread = nullptr;

const int POLLTIMEOUT_MS = 10000;

// 统一事件源核心
// 创建wakeupfd，mainreactor notify the subreactor to slove the corresponding channel
int createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("cant wake up the thread ! error:%d\n", errno);
    }
    return evtfd;
}
EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::t_cachedTid)
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventFd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Exists another EventLoop %p int thread :%d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 给自己的wakeupfd设置回调
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环  必须在IO线程中，我没有写相关断言，需要注意
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n ", this);

    while (!quit_)
    {
        activeChannels_.clear();
        //epoll_wait 没有事件发生会阻塞timeout时间
        pollReturnTime_ = poller_->poll(POLLTIMEOUT_MS, &activeChannels_);

        
        for (Channel *channel : activeChannels_)
        {
            //通知channel处理其相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //这个函数的设计让I/O线程能够执行额外的计算任务，因为如果没有事件发生，I/O线程会一直处于I/O空闲状态，
        //这个时候我们可以利用I/O线程来执行一些额外的任务
        // subreactor被唤醒后执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();  //wakeupfd唤醒后主要就是来做这个的，上面的for循环就简单接收一个8字节数字
    }
    LOG_INFO("EventLoop %p stop looping!", this);
    looping_ = false;
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread()) // 如果此loop的quit在其它线程中被调用
    {
        wakeup(); // 它的quit_已被改变，唤醒然后让loop从poll调用上释放，然后跳出循环结束
    }
}

// 用于mainloop通知subloop，唤醒，读8字节   subloop的handleread就是从fd中读，没实际意义，仅为唤醒
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8\n", n);
    }
}

//依据loop是否在其对应线程而做出操作
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mtx_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应线程
    //第二个条件，为在对应线程中，但处于doPendingFunctors()逻辑中，正在执行回调，
    //但又添加了新的回调，poll处又会阻塞，若不在添加时唤醒，则没有机会执行新的回调
    //第二个条件还未理解......
    //由于doPendingFunctors()调用的Functor可能再次调用queueInLoop(cb)，
    //这时queueInLoop()就必须wakeup(),否则新增的cb可能就不能及时调用了
    if (!isInLoopThread()||callingPendingFunctors_)  
    {
        wakeup();
    }
}

//  给loop对应的wakeupfd写数据就行了
//  wakeupChannel发生读事件，对应线程从poll处的阻塞下来，达到唤醒的效果
void EventLoop::wakeup()
{
    uint64_t tmp = 1;
    ssize_t n = write(wakeupFd_,&tmp,sizeof tmp);

    if(n!=sizeof tmp)
    {
        LOG_ERROR("EventLoop wakeup() writes %d bytes instead of 8",n);
    }
}

//回调执行核心
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        /*
        考虑线程安全问题，因为处理此处vector必须加锁，如果不用局部变量，每次从
        pendingFunctors_中一个个拿再删除，此时mainloop还有可能继续添加回调到subloop对应的pendingFunctors_中.
        那当之前的pendingFunctors_中回调过多的时候，获取不到锁，mainloop直接就阻塞了
        因此直接一个临时变量解决
        */
        std::unique_lock<std::mutex> lock(mtx_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}

// 老朋友，桥接调poller
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

/*
muduo的设计没有使用生产者消费者的线程安全队列来解耦mainloop和subloop，全靠wakeupfd直接通信
理解上难度加大
*/
