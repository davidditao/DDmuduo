// 线程池，用来管理 EventLoopThread
#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>; 

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    // 设置线程的数量
    void setThread(int numThreads) { numThreads_ = numThreads; }

    // 开启整个事件循环线程
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

     // 如果工作在多线程中，baseLoop默认以轮询的方式分配Channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const {return started_;}

    const std::string name()const{return name_;}

private:
    EventLoop *baseLoop_;                                   // 如果不设置线程数量的话，就是单线程，也就是只有 baseLoop_
    std::string name_;                                      // 线程池的名字
    bool started_;                                          //
    int numThreads_;                                        // 线程数量
    int next_;                                              // 指向loops中下一个要被询问的EventLoop
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 包含了所有创建的EventLoopThread,事件循环线程
    std::vector<EventLoop *> loops_;                        // 包含了所有线程中创建的EventLoop
};
