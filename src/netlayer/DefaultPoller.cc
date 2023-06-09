#include "Poller.h"
#include "EpollPoller.h"

/*
此源文件只为实现Poller头文件中声明的static方法newDefaultPoller
由于此方法需要返回一个具体的对象，而muduo库中此处可能有两种对象，poll或epoll
为了避免在基类文件中引用派生类头文件这种不好的做法，故单独开此文件实现

因为基类本来就代表抽象，不能依赖具体的实现
*/
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;   //生成poll实例
    }
    else
    {
        return new EpollPoller(loop);   //生成epoll实例
    }
}

//其实我只实现了epoll，单纯觉得此处实现很好，留下记录