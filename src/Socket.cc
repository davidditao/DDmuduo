#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    close(sockfd_);
}

// 封装一下bind
void Socket::bindAddress(const InetAddress &localaddr)
{
    if (::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)) < 0)
    {
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}

// 封装一下listen
void Socket::listen()
{
    if (::listen(sockfd_, 1024) < 0)
    {
        LOG_FATAL("listen sockfd: %d \n", sockfd_);
    }
}

// 封装一下accept
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
        // 将对端的地址返回
        peeraddr->setSockAddr(addr);
    }

    return connfd;
}

// 关闭sockfd的写端, 封装了一下shutdown
void Socket::shutdownWrite()
{
    // 关闭写端
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error! \n");
    }
}

/**
 * 下面几个函数封装了一下 setsockopt 中的几个选项
 */
// TCP_NODELAY, 禁用 Nagle 算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

// SO_REUSEADDR, 允许 bind 一个进入 TIME_WAIT 状态的地址
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

// SO_REUSEPORT, 允许 bind 一个进入 TIME_WAIT 状态的地址
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

// SO_KEEPALIVE 保活机制
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}
