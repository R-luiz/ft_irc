#include "Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName), topic("") {}

Channel::~Channel() {}

void Channel::addUser(User* user, bool isOperator) {
    if (user) {
        std::cout << "Adding user " << user->getNick() << " to channel " << name << std::endl;
        if (std::find(users.begin(), users.end(), user) == users.end()) {
            users.push_back(user);
            operators[user] = isOperator;
            std::cout << "User " << user->getNick() << " added to channel " << name << std::endl;
            std::cout << "Channel " << name << " now has " << users.size() << " users" << std::endl;
            if (isOperator) {
                std::cout << "User " << user->getNick() << " set as operator in channel " << name << std::endl;
            }
        } else {
            std::cout << "User " << user->getNick() << " is already in channel " << name << std::endl;
        }
    } else {
        std::cerr << "Attempt to add null user to channel " << name << std::endl;
    }
}

void Channel::removeUser(User* user) {
    std::cout << "Attempting to remove user " << (user ? user->getNick() : "NULL") << " from channel " << name << std::endl;
    std::vector<User*>::iterator it = std::find(users.begin(), users.end(), user);
    if (it != users.end()) {
        users.erase(it);
        std::cout << "User " << user->getNick() << " removed from channel " << name << std::endl;
    } else {
        std::cout << "User " << (user ? user->getNick() : "NULL") << " not found in channel " << name << std::endl;
    }
    std::map<User*, bool>::iterator op_it = operators.find(user);
    if (op_it != operators.end()) {
        operators.erase(op_it);
        std::cout << "User " << user->getNick() << " removed from operators list of channel " << name << std::endl;
    }
    std::cout << "Channel " << name << " now has " << users.size() << " users" << std::endl;
}

bool Channel::hasUser(User* user) const 
{
    return std::find(users.begin(), users.end(), user) != users.end();
}

void Channel::setTopic(const std::string& newTopic) 
{
    topic = newTopic;
}

std::string Channel::getTopic() const 
{
	return topic.empty() ? "No topic is set" : topic;
}

std::string Channel::getName() const 
{
    return name;
}

std::vector<User*> Channel::getUsers() const 
{
    return users;
}

bool Channel::isOperator(User* user) const 
{
    std::map<User*, bool>::const_iterator it = operators.find(user);
    return it != operators.end() && it->second;
}

void Channel::setOperator(User* user, bool status) 
{
    operators[user] = status;
}

void Channel::printState() const {
    std::cout << "Channel State for " << name << ":" << std::endl;
    std::cout << "  Topic: " << (topic.empty() ? "No topic set" : topic) << std::endl;
    std::cout << "  Users (" << users.size() << "):" << std::endl;
    for (std::vector<User*>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (*it) {
            std::cout << "    - " << (*it)->getNick() << " (fd: " << (*it)->getFd() << ")";
            if (isOperator(*it)) {
                std::cout << " (operator)";
            }
            std::cout << std::endl;
        } else {
            std::cout << "    - NULL user" << std::endl;
        }
    }
}

void Channel::broadcastMessage(const std::string& message, User* sender)
{
    std::cout << "Broadcasting message in channel " << name << std::endl;
    std::cout << "Number of users in channel: " << users.size() << std::endl;
    for (std::vector<User*>::iterator it = users.begin(); it != users.end(); ++it) {
        if (*it && *it != sender) {
            int userFd = (*it)->getFd();
            std::cout << "Attempting to send to user " << (*it)->getNick() << " (fd: " << userFd << ")" << std::endl;
            if (userFd != -1) {
                ssize_t sent = send(userFd, message.c_str(), message.length(), 0);
                if (sent == -1) {
                    std::cerr << "Error sending message to user " << (*it)->getNick() << ": " << strerror(errno) << std::endl;
                } else if (static_cast<size_t>(sent) < message.length()) {
                    std::cerr << "Incomplete message sent to user " << (*it)->getNick() << std::endl;
                } else {
                    std::cout << "Message sent successfully to user " << (*it)->getNick() << std::endl;
                }
            } else {
                std::cerr << "Invalid fd for user " << (*it)->getNick() << std::endl;
            }
            // Add a small delay between sends
            usleep(10000); // 10ms delay
        } else if (*it == sender) {
            std::cout << "Skipping sender: " << sender->getNick() << std::endl;
        } else {
            std::cout << "Null user encountered in channel" << std::endl;
        }
    }
}

