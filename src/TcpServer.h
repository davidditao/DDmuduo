#pragma once

// 将用户需要的头文件都帮他们导入
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

// 对外的服务器编程使用的类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);

    ~TcpServer();

    // 设置线程初始化回调
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    // 设置各种回调
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompeleteCallback_ = cb; }

    // 设置subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    // TcpConnection相关
    // 有一条新用户的连接来了, 需要做的回调
    void newConnection(int sockfd, const InetAddress &peerAddr);
    // 有一条连接断开,将TcpConnection从connections_中移除
    void removeConnection(const TcpConnectionPtr &conn);
    //
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;                    // baseLoop 用户定义的loop
    const std::string ipPort_;           // 服务器端口号
    const std::string name_;             // 服务器名称
    std::unique_ptr<Acceptor> acceptor_; // 运行在mainLoop中,监听新的连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_;        // 有新连接时的回调
    MessageCallback messageCallback_;              // 有读写消息时的回调
    WriteCompleteCallback writeCompeleteCallback_; // 消息发送完成以后的回调
    ThreadInitCallback threadInitCallback_;        // 对线程进行初始化的回调(在EventLoopThread的threadFunc函数中)

    std::atomic_int started_; //

    int nextConnId_; //

    ConnectionMap connections_; // 保存所有的连接
};