// 在一个Thread中创建一个EventLoop，实现 one loop pre thread
#pragma once

#include "noncopyable.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;
class Thread;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *loop_;              //
    bool exiting_;                 // 是否在退出循环
    Thread thread_;                //
    std::mutex mutex_;             // 互斥锁
    std::condition_variable cond_; // 条件变量
    ThreadInitCallback callback_;  // 线程初始化回调,可以传一个初始化函数来对这个线程进行一个初始化操作
};
