
#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"
#include "EventLoop.h"

#include <sys/types.h>
#include <sys/socket.h>

// 用来初始化accpetSocket
static int createNonblocking()
{
    // SOCK_NONBLOCK:非阻塞, SOCK_CLOEXEC:子进程不继承父进程的fd
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false)
{
    acceptSocket_.setReuseAddr(true); // setsockopt
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    /**
     * 用户在使用TcpServer时，会调用start方法，这个方法会启动Acceptor监听新用户的连接
     * 如果有新用户的连接，需要执行一个回调，将connfd打包成一个Channel，然后唤醒一个subloop
     * 将Channel交给subloop，让subloop来进行后续的操作(客户端的读写事件)
     */
    // bind + this 是为了绑定一个对象的成员函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

// 开始监听
void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.enableReading();
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr; // 客户端地址
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        // 如果用户有注册回调
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        // 出错
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        // EMFILE:当前进程的文件描述符已达上限
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}