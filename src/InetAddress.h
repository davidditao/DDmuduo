#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

class InetAddress
{
public:
	explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
	explicit InetAddress(const sockaddr_in &addr)
		: addr_(addr)
	{
	}

	std::string toIp() const;
	std::string toIpPort() const;
	uint16_t port() const;

	const struct sockaddr_in *getSockAddr() const { return &addr_; }
	void setSockAddr(const sockaddr_in &addr) {addr_ = addr;}
private:
	struct sockaddr_in addr_;
};
