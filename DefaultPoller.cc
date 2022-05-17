#include "Poller.h"

// 注意:在Poller.cc中不实现 newDefaultPoller() 方法
// 原因是 newDefaultPoller()方法需要返回一个 Poller类的派生类: PollPoller 或 EPollPoller
// 那么我们就必须要包含这两个派生类的头文件
// 但注意Poller类是基类,而在基类中包含派生类的头文件是一个非常不好的设计
// 所以我们将 newDefaultPoller()方法写在另一个文件中

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll实例
    }
    else
    {
        return nullptr; // 生成epoll实例
    }
}