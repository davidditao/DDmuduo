#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    //
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    // 虚析构:在多态情况下使子类能够先调用自己的析构函数
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口:创建 更新 删除
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannel) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断Channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该借口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // key:sockfd value:sockfd所属的Channel, 利用unordered_map可以快速查找
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_;  // 该Poller属于哪个EventLoop事件循环   
};