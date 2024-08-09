#include "Client.hpp"

Client::Client() : Fd(-1), authenticated(false), user(new User()), nickset(false), userset(false) {}

Client::Client(const Client& other) : Fd(other.Fd), IPadd(other.IPadd),  authenticated(other.authenticated),
	  user(new User(*other.user)), buffer(other.buffer) 
{
		user->setFd(Fd);
}

Client::~Client()
{
	delete user;
}

Client& Client::operator=(const Client& other) {
	if (this != &other) 
	{
		Fd = other.Fd;
		IPadd = other.IPadd;
		delete user;
		user = new User(*other.user);
		buffer = other.buffer;
		authenticated = other.authenticated;
	}
	return *this;
}

void Client::setIpAdd(std::string ipadd)
{
	IPadd = ipadd;
}

void Client::setFd(int fd)
{
	Fd = fd;
	if (user)
		user->setFd(fd);
}

int Client::getFd()
{
	return Fd;
}

void Client::appendToBuffer(const std::string& data)
{
	buffer += data;
}

void Client::clearBuffer()
{
	buffer.clear();
}

std::string& Client::getBuffer()
{
	return buffer;
}

bool Client::isAuthenticated()
{
	return authenticated;
}

void Client::setAuthenticated(bool auth)
{
	authenticated = auth;
}

User* Client::getUser()
{
	return user;
}

std::string Client::getIPadd()
{
	return IPadd;
}

void Client::setIPadd(std::string ipadd)
{
	IPadd = ipadd;
}

void Client::setUser(User* user)
{
	if (this->user)
		delete this->user;
	this->user = user;
	if (Fd != -1)
		user->setFd(Fd);
}

bool Client::isNickSet()
{
	return user->getNick().length() > 0;
}

void Client::setNickSet(bool set)
{
	this->nickset = set;
}

bool Client::isUserSet()
{
	return this->userset;
}

void Client::setUserSet(bool set)
{
	this->userset = set;
}
