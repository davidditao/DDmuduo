#pragma once

#include "Poller.h"
#include <sys/epoll.h>

/**
 * epoll的使用：
 *      epoll_create                        -> EPollPoller()
 *      epoll_ctl:                          -> update() 
 *          EPOLL_CTL_ADD, EPOLL_CTL_MOD    -> updateChannel()
 *          EPOLL_CTL_DEL                   -> removeChannel()
 *      epoll_wait                          -> Poll()
 */

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类的方法
    // epoll的 创建 更新 删除
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    // EventList的初始长度
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新Channel, epoll_ctl
    void update(int opertion, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;      // epoll_wait的第一个参数
    EventList events_; // epoll_wait的第二个参数
};