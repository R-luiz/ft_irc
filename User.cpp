#include "User.hpp"

User::User() : nickname(""), username(""), hostname(""), realname(""), fd(-1) {}

User::User(std::string nick, std::string user, int fd) : nickname(nick), username(user), fd(fd) {}

User::~User() {}

std::string User::getNick() 
{
	return nickname.empty() ? "Unknown" : nickname;
}

std::string User::getUser() 
{
	return username.empty() ? "Unknown" : username;
}

std::string User::getHostname() 
{
	return hostname.empty() ? "Unknown" : hostname;
}

int User::getFd() 
{
	return fd;
}

void User::setNick(std::string nick) 
{
	nickname = nick;
}

void User::setUser(std::string user) 
{
	username = user;
}

void User::setHostname(std::string host) 
{
	hostname = host;
}

void User::setFd(int fd) 
{
	this->fd = fd;
}

void User::setRealName(std::string real) 
{
	realname = real;
}

std::string User::getRealName() 
{
	return realname;
}