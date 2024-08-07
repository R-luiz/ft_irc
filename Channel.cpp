#include "Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName) {}

Channel::~Channel() {}

void Channel::addUser(User* user) {
    users.push_back(user);
    operators[user] = false;
}

void Channel::removeUser(User* user) {
    users.erase(std::remove(users.begin(), users.end(), user), users.end());
    operators.erase(user);
}

bool Channel::hasUser(User* user) const {
    return std::find(users.begin(), users.end(), user) != users.end();
}

void Channel::setTopic(const std::string& newTopic) {
    topic = newTopic;
}

std::string Channel::getTopic() const {
    return topic;
}

std::string Channel::getName() const {
    return name;
}

std::vector<User*> Channel::getUsers() const {
    return users;
}

bool Channel::isOperator(User* user) const {
    std::map<User*, bool>::const_iterator it = operators.find(user);
    return it != operators.end() && it->second;
}

void Channel::setOperator(User* user, bool status) {
    operators[user] = status;
}

void Channel::broadcastMessage(const std::string& message, User* sender) {
    std::vector<User*>::iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        User* user = *it;
        if (user != sender) {
            int userFd = user->getFd();
            if (userFd != -1) {
                ssize_t bytesSent = send(userFd, message.c_str(), message.length(), 0);
                if (bytesSent == -1) {
                    std::cerr << "Error sending message to user " << user->getNick() << std::endl;
                } else if (bytesSent < static_cast<ssize_t>(message.length())) {
                    std::cerr << "Incomplete message sent to user " << user->getNick() << std::endl;
                }
            } else {
                std::cerr << "Invalid file descriptor for user " << user->getNick() << std::endl;
            }
        }
    }
}
