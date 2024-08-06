#include "User.hpp"

User::User() {}

User::User(std::string nick, std::string user, std::string pass) {
	nickname = nick;
	username = user;
	password = pass;
}

User::~User() {}

std::string User::getNick() {
	return nickname;
}

std::string User::getUser() {
	return username;
}

std::string User::getPass() {
	return password;
}

std::string User::getHostname() {
	return hostname;
}

int User::getFd() {
	return fd;
}

void User::setNick(std::string nick) {
	nickname = nick;
}

void User::setUser(std::string user) {
	username = user;
}

void User::setPass(std::string pass) {
	password = pass;
}

void User::setHostname(std::string host) {
	hostname = host;
}

void User::setFd(int fd) {
	this->fd = fd;
}