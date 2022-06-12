#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
               const InetAddress &addr,
               const std::string &name)
        : server_(loop, addr, name),
          loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的loop线程数量
        server_.setThreadNum(3); // 加上mainLoop是4个
    }

    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            // 连接建立
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            // 连接断开
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        // 接收到的消息
        std::string msg = buf->retrieveAllAsString();
        // echo:将这个消息发送回给客户端
        conn->send(msg);
        // 服务器主动关闭，关闭写端： EPOLLHUP -> closeCallback_
        conn->shutdown();
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    // mainLoop
    EventLoop loop;
    // 指定端口号
    InetAddress addr(8000);
    // 创建服务器对象: non-blocking listenfd create bind
    EchoServer server(&loop, addr, "EchoServer");
    // 开启服务器: listen loopthread listenfd->acceptChannel->mainLoop
    server.start();
    // 启动mainLoop的Poller
    loop.loop();

    return 0;
}