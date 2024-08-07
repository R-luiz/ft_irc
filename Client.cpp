#include "Client.hpp"

Client::Client() {}

void Client::setIpAdd(std::string ipadd)
{
	IPadd = ipadd;
}

void Client::SetFd(int fd)
{
	Fd = fd;
}

int Client::getFd()
{
	return Fd;
}