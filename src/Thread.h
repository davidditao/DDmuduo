// 封装一个线程类，一个Thread对象，记录的就是一个新线程的详细信息
#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <unistd.h>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    int join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_; // 开始线程
    bool joined_;  // 主线程等待所有线程运行完毕
    /**
     * 这里不能直接用C++的 std::thread thread_;
     * 因为如果一调用就直接启动线程了
     * 这里用智能指针来封装
     */
    std::shared_ptr<std::thread> thread_;   // 创建线程
    pid_t tid_;                             // 线程id
    ThreadFunc func_;                       // 线程函数
    std::string name_;                      // 线程的名字
    static std::atomic_int32_t numCreated_; // 记录产生线程的个数(静态变量)
};