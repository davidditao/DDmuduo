#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    /**
     * 线程中绑定的EventLoop都是栈上的对象，所以不需要在析构函数中delete
     */
}

// 开启整个事件循环线程, cb 为用户定义的每个EventLoopThread的初始化函数
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    // 创建numThreads_个线程
    for (int i = 0; i < numThreads_; i++)
    {
        // 给每个线程命名
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        // 创建一个 EventLoopThread
        EventLoopThread *t = new EventLoopThread(cb, buf);
        // 将EventLoopThread存入threads_中
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        /**
         * EventLoopThread 中的startLoop()会创建一个子线程，
         * 这个子线程中会创建一个与之对应的 EventLoop对象
         * startLoop() 会返回这个对象
         * 将这个对象存入loop_中
         */
        loops_.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程，运行着baseLoop
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop默认以轮询的方式分配Channel给subloop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;

    // 如果是多线程,通过轮询的方式从线程池中获取下一个Eventloop
    if(!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

// 获得所有的loop
std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else 
    {
        return loops_;
    }
}