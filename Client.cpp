#include "Client.hpp"

Client::Client() : Fd(-1), authenticated(false), user(new User()), nickset(false), userset(false) {
	user->setFd(Fd);
}

Client::Client(const Client& other) : Fd(other.Fd), IPadd(other.IPadd), 
          authenticated(other.authenticated),
          user(new User(*other.user)), 
          nickset(other.nickset),
          userset(other.userset),
          buffer(other.buffer)
{
    user->setFd(Fd);
}

Client& Client::operator=(const Client& other) {
        if (this != &other) {
            Fd = other.Fd;
            IPadd = other.IPadd;
            authenticated = other.authenticated;
            delete user;
            user = new User(*other.user);
            buffer = other.buffer;
            nickset = other.nickset;
            userset = other.userset;
            user->setFd(Fd);
        }
        return *this;
    }

Client::~Client() {
        delete user;
    }

void Client::setIpAdd(std::string ipadd)
{
	IPadd = ipadd;
}

std::string Client::getBuffer() const
{
	return buffer;
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

User* Client::getUser() const
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

bool Client::isConnected() const
{
    if (Fd == -1) return false;
    
    // Check if the file descriptor is valid
    int error = 0;
    socklen_t len = sizeof(error);
    int retval = getsockopt(Fd, SOL_SOCKET, SO_ERROR, &error, &len);
    
    if (retval == 0 && error == 0) {
        // The socket is still connected
        return true;
    } else {
        std::cerr << "Client " << getUser()->getNick() << " is no longer connected. Error: " << strerror(error) << std::endl;
        return false;
    }
}