#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <memory>

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd,用来唤醒subReactor处理新来的Channel
int createEventfd()
{
    // eventfd(),用于进程间通信
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // mainReactor通过给subReactor写东西（handleRead）通知subReactor要起来了
    // 设置wakeupfd的事件类型以及发生事件后的回调操作,也就是读一个8字节的数据
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd，一类是client的fd，一类是wakeupfd(mainloop用来唤醒subloop)
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        // 处理activeChannels
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些Channel发生事件了，然后上报给EventLoop，通知Channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程(mainReactor)只接收新用户的连接，并将accept返回的fd封装到一个Channel中
         * 而已连接用户的Channel要分发给subloop
         * 所以mainloop要事先注册一个回调cb(需要subloop来执行)
         * mainloop通过eventfd唤醒subloop后，会执行上面的handleEvent，构造函数中将他注册为handleRead，也就是读一个8字节的数据，用来唤醒subloop
         * 然后执行下面的doPendingFunctors()，也就是mainloop事先注册的回调操作cb。
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

/**
 * 退出事件循环:
 *  1. loop在自己的线程中调用quit
 *  2. 在其他线程中调用quit。比如在一个subloop中，调用了mainloop的quit，要先将mainloop唤醒，mainloop中的loop()才会结束
 */
void EventLoop::quit()
{
    quit_ = true;
    // 如果在其他线程中，那要先将他唤醒
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前Loop中执行回调
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 如果在当前loop线程中，执行cb
    {
        cb();
    }
    else // 如果在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

// 把回调放入队列中，Loop所在的线程被唤醒后再执行回调
void EventLoop::queueInLoop(Functor cb)
{
    // 多个loop中可能同时执行另一个loop中的runInLoop()，所以需要锁
    {
        // 智能锁
        std::unique_lock<std::mutex> lock(mutex_);
        // push_back是拷贝构造，emplace_back是直接构造。将回调写入队列中。
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒loop所在的线程:
    // 1. 如果不在当前线程中
    // 2. 在当前线程中，但当前线程正在执行回调，现在又写入了新的回调，那么上一轮的回调执行完后，不阻塞，直接唤醒，继续执行这一轮的回调
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); 
    }
}

// 唤醒的subloop起来之后，读一个8字节的数据
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes intstead of 8 \n", n);
    }
}

// mainReactor 用来唤醒 subReactor: 向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n= write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 => Poller的方法
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
    return poller_->hasChannel(channel);
}

// 执行回调
void EventLoop::doPendingFunctors()
{
    // 很好的思想!!!
    std::vector<Functor> functors;
    // 开始执行回调
    callingPendingFunctors_ = true;     
    // 多个loop中可能同时执行，需要加锁
    {
        // 智能锁
        std::unique_lock<std::mutex> lock(mutex_);
        /**
         * 将pendingFunctors和刚定义的空数组functors交换
         * 得到所有的数据的同时，也将pendingFunctors_置空了
         * 这样做的好处是：可以很快地将锁释放，让其他线程可以继续处理。
         * 而本线程得到所有数据后，也可以在后面慢慢处理
         */
        functors.swap(pendingFunctors_);
    }

    // 将队列中的回调拿出来执行
    for(const Functor& functor : functors)
    {
        functor();
    }
    // 回调执行完毕
    callingPendingFunctors_ = false;
}