#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*	基于muduo网络库开发服务器程序
	1. 组合TcpServer对象
	2. 创建EventLoop事件循环对象的指针
	3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
	4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
	5. 设置合适的服务端线程数量，muduo库会自己划分IO线程和工作线程
	6. 开启事件循环函数

*/

class ChatServer {	// 自己定义一个服务器的类
public:
	// #3
	ChatServer(EventLoop *loop,								// 事件循环
			const InetAddress& listenAddr,					// IP+Port
			const string &nameArg)							// 服务器的名字
		:_server(loop, listenAddr, nameArg), _loop(loop)
	{
		// #4 给服务器注册用户连接的创建和断开回调
		_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

		// #4 给服务器注册用户读写事件回调
		_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

		// #5 设置服务端的线程数量 1个IO线程，3个工作线程
		_server.setThreadNum(4);
	
	}

	// #6 开启事件循环
	void start(){	
		_server.start();
	}

private:
	// #4 专门处理用户的连接创建和断开
	void onConnection(const TcpConnectionPtr &conn){
		if(conn->connected()){
			cout << conn->peerAddress().toIpPort() << " -> " 
				<< conn->localAddress().toIpPort() << " state:online " << endl;
		} else {
			cout << conn->peerAddress().toIpPort() << " -> " 
				<< conn->localAddress().toIpPort() << " state:offline " << endl;
			conn->shutdown();		// close(fd)
			// _loop->quit();
		}		
	}

	// #4 专门处理用户的读写事件
	void onMessage(const TcpConnectionPtr &conn,		// 连接
					Buffer *buffer,						// 缓冲区
					Timestamp time)						// 接受到数据的时间信息
	{
		string buf = buffer->retrieveAllAsString();
		cout << "recv data:" << buf << "time:" << time.toString() <<endl;
		conn->send(buf);
	}

	TcpServer _server;				// #1
	EventLoop *_loop;				// #2

};

int main(){
	EventLoop loop;		// epoll
	InetAddress addr("127.0.0.1", 1997);
	ChatServer server(&loop, addr, "ChatServer");

	server.start();		// epoll_ctl将listenfd 添加到epoll上
	loop.loop();		// epoll_wait 以阻塞方式等待新用户连接，已连接用户的读写事件等

	exit(0);
}
