#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 静态的成员变量要在类外单独初始化
std::atomic_int32_t numCreated_;

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    /**
     * 1. 线程已经运行了才需要析构
     * 2. joined_表示主线程需要等待子线程结束
     * 3. 如果不是joined_线程的话就是守护线程，主线程就不需要等待
     */

    if (started_ && !joined_)
    {
        // 分离线程
        thread_->detach();
    }
}

// 启动一个线程
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    /**
     * 创建一个线程对象，thread_ 指针指向这个线程对象
     * 使用lambda表达式，设置为引用捕获，以引用的方式接受外部的线程对象
     * 这样就可以访问线程的成员变量了
     */
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid
        tid_ = CurrentThread::tid();
        // 给信号量加一
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        func_();
    }));

    /**
     * 这里必须等待获取新创建的子线程的tid
     * 使用信号量 
     */
    sem_wait(&sem);

}
int Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
