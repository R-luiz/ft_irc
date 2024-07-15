#include "Client.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

Client::Client(int fd) : fd(fd), authenticated(false) {}

Client::~Client() {
    close(fd);
}

int Client::getFd() const { return fd; }
const std::string& Client::getNickname() const { return nickname; }
const std::string& Client::getUsername() const { return username; }
bool Client::isAuthenticated() const { return authenticated; }

void Client::setNickname(const std::string& nick) { nickname = nick; }
void Client::setUsername(const std::string& user) { username = user; }
void Client::setPassword(const std::string& pwd) { password = pwd; }

bool Client::authenticate(const std::string& pwd) {
    authenticated = (password == pwd);
    return authenticated;
}

void Client::sendMessage(const std::string& message) {
    std::string fullMessage = message + "\r\n";
    send(fd, fullMessage.c_str(), fullMessage.length(), 0);
}

bool Client::isConnected() const {
    char buf;
    int result = recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    return !(result == 0 || (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK));
}

void Client::setAuthenticated(bool auth) {
    authenticated = auth;
}

const std::string& Client::getPassword() const {
    return password;
}

std::string Client::getHostname() const {
    return hostname;
}
