#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <string.h>

// 用于TcpServer的初始化，用户传入的loop不能为空
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1)
{
    // 如果有新用户连接时，会执行newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    // 析构:将所有的connection释放
    for (auto &item : connections_)
    {
        // 这个局部的强智能指针出了作用域就会自动释放
        TcpConnectionPtr conn(item.second);
        // 使connections_中的智能指针(即item.second)不再指向TcpConnection对象
        item.second.reset();
        // 调用TcpConnection::connectionDestroyed来销毁这个连接
        conn->getLoop()->runInLoop(std::bind(TcpConnection::connectDestroyed, conn));
    }
    // 出了这个作用域 conn 指针也自动释放，至此，没有任何指针指向TcpConnection对象了
}

// 设置subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    // started_为原子的，防止一个TcpServer对象被start多次
    if (started_++ == 0)
    {
        // 启动线程池
        threadPool_->start(threadInitCallback_);
        // 唤醒subLoop后执行listen()。get()方法是将unique_ptr变为一个普通指针
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// TcpConnection相关
// 有一条新用户的连接来了, 需要做的回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法，选择一个subLoop，来管理Channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    // 连接名
    char buf[64] = {0};
    // 字符串拼接
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr));

    connections_[connName] = conn;
    // 下面的回调都是用户设置的 TcpServer -> TcpConnecton -> Channel -> Poller -> 通知Channel执行回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompeleteCallback_);
    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    // 直接调用TcpConnectoin::connectEstablished方法，建立连接
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

// 有一条连接断开,将TcpConnection从connections_中移除
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

//
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
             name_.c_str(), conn->name().c_str());

    // 将conn从connections_中删除
    connections_.erase(conn->name());
    // 获取conn所在的loop
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
