#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

#include <functional>

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

// 发送数据
void TcpConnection::send(const void *message, int len)
{
}

// 关闭当前连接
void TcpConnection::shutdown()
{
}

// 连接建立
void TcpConnection::connectEstablished() {}
// 连接销毁
void TcpConnection::connectDestroyed() {}

// 回调
void TcpConnection::handleRead(Timestamp receiveTime)
{
}
void TcpConnection::handleWrite()
{
}
void TcpConnection::handleClose()
{
}
void TcpConnection::handleError()
{
}

void TcpConnection::sendInLoop(const void *message, size_t len) {}
void TcpConnection::shutdownInLoop() {}
