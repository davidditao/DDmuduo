#include "InetAddress.h"

#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
	bzero(&addr_, sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr.s_addr);
}

std::string InetAddress::toIp() const
{
	char buf[64];
	inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
	return buf;
}

std::string InetAddress::toIpPort() const
{
	// ip::port
	char buf[64];
	inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
	size_t end = strlen(buf);
	uint16_t port = ntohs(addr_.sin_port);
	sprintf(buf + end, ":%u", port);
	return buf;
}

uint16_t InetAddress::port() const
{
	return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main()
// {
// 	InetAddress addr(8080);
// 	std::cout << addr.toIpPort() << std::endl;
// 	return 0;
// }
