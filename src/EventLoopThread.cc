#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        // 退出EventLoop
        loop_->quit();
        // 等待子线程退出
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    // 启动新线程,这个新线程会运行下面的threadFunc方法, 创建一个EventLoop
    thread_.start();

    // 主线程继续往下运行
    EventLoop *loop = nullptr;
    // 子线程中会创建一个 EventLoop,并放在 loop_ 中。主线程要获取这个EventLoop对象并返回
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            // 等待子线程处理完毕
            cond_.wait(lock);
        }
        loop = loop_;
    }
    // 将子线程中创建的EventLoop返回
    return loop;
}

// 下面这个方法，是在新线程里面运行的：创建一个 EventLoop
void EventLoopThread::threadFunc()
{
    // 创建一个独立的EventLoop,和上面的线程是一一对应的。one loop pre thread
    EventLoop loop;

    // 如果传了初始化函数
    if (callback_)
    {
        callback_(&loop);
    }

    // 修改成员变量loop_ 
    {
        // 修改loop_前先加锁,防止主线程访问
        std::unique_lock<std::mutex> lock(mutex_);
        /**
         * EventLoopThread这个类其实就是将新创建的 Thread 和 Thread中创建的EventLoop 绑定起来
         * 成员变量 thread_ 就是新创建的线程
         * 成员变量 loop_ 就是新线程中创建的EventLoop
         * 这样就实现了 one loop pre thread
         */
        loop_ = &loop;
        // 修改完毕通知主线程(在startLoop()中）
        cond_.notify_one();
    }
    // 开启事件循环，阻塞在Poller，等待事件到来
    loop.loop();
    // 这一对 [Thread, EventLoop] 运行完毕(服务器关闭)，将loop_清空，可以进行下一对的创建
    std::unique_lock<std::mutex> lock(mutex_);  // 访问loop_都需要加锁
    loop_ = nullptr;
}
