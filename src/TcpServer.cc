#include "TcpServer.h"
#include "Logger.h"

// 用于TcpServer的初始化，用户传入的loop不能为空
EventLoop *CheckLoopNotNull(EventLoop *loop)
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
    if(started_++ == 0)
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

}
// 有一条连接断开,将TcpConnection从connections_中移除
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
}

//
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
}
