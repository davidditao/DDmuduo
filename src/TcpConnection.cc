#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <string>

// 用于TcpConnection的初始化，用户传入的loop不能为空
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 注册回调
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);

    // 启动保活机制
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}



// 连接建立
void TcpConnection::connectEstablished()
{
    // 设置连接状态
    setState(kConnected);
    // 防止回调在执行的时候，TcpConnection被销毁了
    channel_->tie(shared_from_this());
    // 向poller注册读事件
    channel_->enableReading();

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    // 已建立的连接才能关闭
    if (state_ == kConnected)
    {
        // 设置连接状态
        setState(kDisconnected);
        // 把channel的所有感兴趣的事件从poller中删除
        channel_->disableAll();
        // 执行回调
        connectionCallback_(shared_from_this());
    }
    // 把channel从poller中删除
    channel_->remove();
}

// 回调
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        // 客户端断开
        handleClose();
    }
    else
    {
        // 出错
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    // 如果可读
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            // 如果写成功，更新指针位置
            outputBuffer_.retrieve(n);
            // 如果发送完成
            if (outputBuffer_.readableBytes() == 0)
            {
                // 不可写
                channel_->disableWriting();
                // 执行回调
                if (writeCompeleteCallback_)
                {
                    // 唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(std::bind(writeCompeleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
            else
            {
                // 出错
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
    }
    else
    {
        // 如果不可读
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

// poller->channel::closeCallback->TcpConnection::handleClose->TcpServer::removeConnection->TcpConnection::connectDestroyed
void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 关闭连接的回调,执行的是TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

// 发送数据
void TcpConnection::send(const std::string &buf)
{
    // 如果已经连接
    if (state_ == kConnected)
    {
        // 是否在当前线程
        if (loop_->isInLoopThread())
        {
            // 如果在当前线程，直接发送
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            // 如果不在，转到loop_的线程，然后发送
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()));
        }
    }
}

/**
 * 发送数据，应用写得快，而内核发送数据慢
 * 需要把待发送数据写入缓冲区，并设置水位回调
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过TcpConnection::shudown，不能再进行发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // 表示channel_第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            // 发送成功
            // 记录剩余还未发送的数据长度
            remaining = len - nwrote;
            // 如果全发送完了,调用写完后的回调
            if (remaining == 0 && writeCompeleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了，也就不用执行handlewirte函数了
                loop_->queueInLoop(std::bind(writeCompeleteCallback_, shared_from_this()));
            }
        }
        else
        {
            // 出错
            nwrote = 0;
            // 如果不是EWOULDBLOCK:非阻塞IO，还没有数据导致的返回
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                // 如果是因为连接重置
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    /**
     * 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel注册epollout事件，
     * 因为epoll工作在LT模式，poller发现发送缓冲区有数据，会通知相应的sock/channel，调用writeCallback_回调
     * 也就是调用TcpConnection::handleWrite方法，把缓冲区中的数据全部发送完成
     */
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送的数据长度
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果需要调整高水位
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // 将剩余数据写入缓冲区
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout
            channel_->enablewriting();
        }
    }
}

// 关闭当前连接
void TcpConnection::shutdown()
{
    // 如果连接是关闭状态
    if(state_ == kConnected)
    {
        // 设置连接状态
        setState(kDisconnecting);
        // 在自己的线程中执行回调
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    // 如果不可写，说明已经将发送缓冲区outputBuffer的数据发送完了
    if(!channel_->isWriting())
    {
        // 关闭写端
        socket_->shutdownWrite();
    }
}
