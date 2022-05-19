#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// Channel的状态, Channel中的index_成员变量
const int kNew = -1;    // 还没有加入到epoll中
const int kAdded = 1;   // 已经添加到epoll中
const int kDeleted = 2; // 已经从epoll中删除了

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),                             // 基类的构造函数(因为基类没有默认构造函数,所以要先显式调用）
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)), // 注意是epoll_create1,EPOLL_CLOEXEC表示exec的时候不会关闭父进程的资源
      events_(kInitEventListSize)               // 初始化events_数组的大小,默认为16
{
    // 报错
    if (epollfd_ < 0)
    {
        LOG_FATAL("EPollPoller::EPollPoller : %s\n", strerror(errno));
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// epoll_wait方法，传入activeChannels，填入发生事件的Channel后返回
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("func = %s : fd total count = %lu \n ", __FUNCTION__, channels_.size());
    // 注意第二个参数的处理方法 &*events_.begin() -> epoll_event*
    int numEvents = epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        // 将发生事件的Channel填入activeChannels
        fillActiveChannels(numEvents, activeChannels);
        // 如果事件数组满了，扩容
        if (numEvents == events_.size())
        {
            // resize会在后面补零
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        // 啥事没有
    }
    else
    {
        // 报错
        if(saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// 将发生事件的Channel填入activeChannels
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        // 拿到发生事件的Channel
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        
        // 将该发生事件的状态填入Channel的revents
        channel->set_revents(events_[i].events);

        // 将Channel填入activeChannels返回
        activeChannels->push_back(channel);
    }
}

/**
 *          EventLoop
 *  ChannelList     Poller
 *                 ChannelMap<fd, Channel*>
 */
// Channel update remove => EvnentLoop updateChannel removeChannel => Poller updateChannel removeChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s : fd = %d events = %d index = %d \n ", __FUNCTION__, channel->fd(), channel->events(), index);

    // 如果需要添加
    if (index == kNew || index == kDeleted)
    {
        // 添加到Poller中
        int fd = channel->fd();
        channels_[fd] = channel; // ChannelMap<fd, Channel*> channels_

        // 设置为已添加
        channel->set_index(kAdded);

        // 添加到epoll中
        update(EPOLL_CTL_ADD, channel);
    }
    else // 如果已经添加了，想要更改
    {
        // 如果channel对任何事件都不感兴趣了，那就删除
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else // 否则更新
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    LOG_INFO("func = %s : fd = %d \n ", __FUNCTION__, fd);
    channels_.erase(fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 更新Channel, epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    // 新建一个epoll_event结构体，用来更新epoll, 作为epoll_ctl的最后一个参数
    struct epoll_event event;
    bzero(&event, sizeof(event));
    
    // 需要修改状态的fd
    int fd = channel->fd();

    // 设置需要更新的fd的状态
    event.events = channel->events();
    
    // 绑定fd和channel，这样通过event就可以拿到fd对应的channel
    event.data.fd = fd;
    event.data.ptr = channel;

    
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl op = EPOLL_CTL_DEL, fd = %d", fd);
        }
        else
        {
            LOG_FATAL("epoll_ctl op = EPOLL_CTL_ADD/EPOLL_CTL_MOD, fd = %d", fd);
        }
    }
}
