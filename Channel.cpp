#include "Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName), topic("") {}

Channel::~Channel()
{
    users.clear();
    operators.clear();
}

void Channel::addUser(User* user, bool isOperator)
{
    if (user && !user->getNick().empty())
    {
        users[user->getNick()] = user;
        operators[user->getNick()] = isOperator;
        std::cout << "Added user " << user->getNick() << " to channel " << name << std::endl;
    }
    else
    {
        std::cerr << "Attempted to add invalid user to channel " << name << std::endl;
    }
}

void Channel::removeUser(const std::string& nickname)
{
    users.erase(nickname);
    operators.erase(nickname);
}

bool Channel::hasUser(const std::string& nickname) const
{
    std::cout << "Checking if user " << nickname << " is in channel " << name << std::endl;
    return users.find(nickname) != users.end();
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

std::map<std::string, User*> Channel::getUsers() const 
{
    return users;
}

bool Channel::isOperator(const std::string& nickname) const 
{
    std::map<std::string, bool>::const_iterator it = operators.find(nickname);
    return it != operators.end() && it->second;
}

void Channel::setOperator(const std::string& nickname, bool status) 
{
    operators[nickname] = status;
}

void Channel::broadcastMessage(const std::string& message, User* sender)
{
    std::cout << "Broadcasting message in channel " << name << std::endl;
    for (std::map<std::string, User*>::const_iterator it = users.begin(); it != users.end(); ++it) 
    {
        User* user = it->second;
        if (user && user != sender)
        {
            try {
                std::string nick = user->getNick();
                int userFd = user->getFd();
                std::cout << "Attempting to send message to user " << nick << " with fd " << userFd << std::endl;
                
                if (userFd != -1 && !message.empty()) {
                    ssize_t sent = send(userFd, message.c_str(), message.length(), 0);
                    if (sent == -1) {
                        std::cerr << "Error sending message to user " << nick << ": " << strerror(errno) << std::endl;
                    } else if (static_cast<size_t>(sent) < message.length()) {
                        std::cerr << "Partial message sent to user " << nick << std::endl;
                    } else {
                        std::cout << "Message sent successfully to user " << nick << std::endl;
                    }
                } else {
                    std::cerr << "Invalid fd or empty message for user " << nick << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception caught while sending message to " << it->first << ": " << e.what() << std::endl;
            }
        }
    }
}