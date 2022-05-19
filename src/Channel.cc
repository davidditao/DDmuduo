#include "Channel.h"

#include <sys/epoll.h>

#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}

Channel::~Channel() {}

//
void Channel::tie(const std::shared_ptr<void> &obj)
{
  tie_ = obj; // 用weak_ptr来接收
  tied_ = true;
}

// 当改变Channel所表示fd的events事件后，update负责在Poller里面更改fd相应的事件epoll_ctl
void Channel::update()
{
  // Channel访问不到Poller，但Channel和Poller都属于一个EventLoop
  // 一个EventLoop包含一个Poller和多个Channel
  // 所以可以通过Channel所属的EventLoop，来调用Poller相应方法，注册fd的events事件
  // add code..
  // loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中，把当前的Channel删除掉
void Channel::remove()
{
  // add code..
  // loop_->removeChannel(this);
}

// fd得到Poller通知以后，处理事件的函数
void Channel::handleEvent(Timestamp receiveTime)
{
  if (tied_)
  {
    std::shared_ptr<void> guard = tie_.lock();
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

// 根据Poller通知发生的具体事件,由Channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  LOG_INFO("Channel handleEvent revents : %d\n", revents_);

  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
  {
    if (closeCallback_)
    {
      closeCallback_();
    }
  }

  if (revents_ & EPOLLERR)
  {
    if (errorCallback_)
    {
      errorCallback_();
    }
  }

  if (revents_ & (EPOLLIN | EPOLLPRI))
  {
    if (readCallback_)
    {
      readCallback_(receiveTime);
    }
  }

  if (revents_ & EPOLLOUT)
  {
    if (writeCallback_)
    {
      writeCallback_();
    }
  }
}
