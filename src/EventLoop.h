#pragma once

#include <functional>
#include <vector>
#include <atomic> // 原子类型
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 时间循环类：主要包含了两大模块 多个Channel 和 一个Poller
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    // Poller返回当前事件的Channel的时间点
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    // 在当前Loop中执行回调
    void runInLoop(Functor cb);
    // 把回调放入队列中，Loop所在的线程被唤醒后再执行回调
    void queueInLoop(Functor cb);

    // mainReactor 用来唤醒 subReactor
    void wakeup();

    // EventLoop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断当前线程是否为创建EventLoop的线程
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    // wakeup 
    void handleRead();
    // 执行回调
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;
    
    // 原子操作，用过CAS实现
    std::atomic_bool looping_;  // 标志：事件循环是否正常进行，或退出循环
    std::atomic_bool quit_;     // 标志：退出loop循环
    
    const pid_t threadId_;    // 记录当前loop所在的线程id

    Timestamp pollReturnTime_;  // Poller返回发生事件的Channel的时间点
    std::unique_ptr<Poller> poller_;    // 一个EventLoop中只有一个Poller
    
    int wakeupFd_;  // (通知机制：eventfd函数) 作用：当mainLoop获取一个新用户的Channel，通过轮询算法选择一个subLoop，通过该成员变量唤醒subLoop。
    std::unique_ptr<Channel> wakeupChannel_;    // 唤醒的Channel

    ChannelList activeChannels_;    // 一个EventLoop中有多个Channel

    std::atomic_bool callingPendingFunctors_;   // 标志：当前Loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;      // 存储Loop需要执行的所有回调操作
    std::mutex mutex_;  // 互斥锁，用来保护上面vector容器的线程安全操作
};