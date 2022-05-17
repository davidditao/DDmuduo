#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

// 头文件中只给出类型的声明，源文件中再包含具体的头文件。
// 使用这种写法，在头文件中只能出现这个类的指针。
class EventLoop;

/*
 *   每个Channel对象自始至终只负责一个文件描述符(fd)的IO事件分发。
 *   Channel会把不同的IO事件分发为不同的回调。
 */

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到Poller通知以后，处理事件的函数
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }

    // Polloer使用set_revents返回具体发生的事件
    int set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    // 让fd对读事件感兴趣
    void enaleReading()
    {   
        // 让 events_ 相应的位置上 kREadEvent
        events_ |= kReadEvent;
        update();
    }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enablewriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前事件的状态
    bool isNoneEvent()const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 当前Channel属于哪个EventLoop
    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 该Channel属于哪个EventLoop事件循环
    const int fd_;    // Poller监听的对象
    int events_;      // 注册fd感兴趣的事件，由用户设置
    int revents_;     // Poller返回的具体发生的事件，由EventLoop/Poller设置
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面能够获知fd最终发生的具体事件revents, 所以它负责调用事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};