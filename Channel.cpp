#include "Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName), topic("") {}

Channel::~Channel()
{
    users.clear();
    operators.clear();
}

void Channel::addUser(User* user, bool isOperator)
{

	if (user && std::find(users.begin(), users.end(), user) == users.end()) 
    {
		users.push_back(user);
		operators[user] = false;
        if (isOperator) 
        {
            operators[user] = true;
        }
    }
}

void Channel::removeUser(User* user) 
{
    users.erase(std::remove(users.begin(), users.end(), user), users.end());
    operators.erase(user);
}

bool Channel::hasUser(User* user) const 
{
    if (!user) return false;
    for (std::vector<User*>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (*it && (*it)->getNick() == user->getNick())
            return true;
    }
    return false;
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

void Channel::broadcastMessage(const std::string& message, User* sender)
{
    std::cout << "Broadcasting message in channel " << name << std::endl;
    for (std::vector<User*>::iterator it = users.begin(); it != users.end(); ++it) 
    {
        if (*it && *it != sender)
        {
            int userFd = (*it)->getFd();
            if (userFd != -1) {
                ssize_t sent = send(userFd, message.c_str(), message.length(), 0);
                if (sent == -1) {
                    std::cerr << "Error sending message to user " << (*it)->getNick() << ": " << strerror(errno) << std::endl;
                } else {
                    std::cout << "Message sent successfully to user " << (*it)->getNick() << std::endl;
                }
            } else {
                std::cerr << "Invalid fd for user " << (*it)->getNick() << std::endl;
            }
        } else if (*it == sender) {
            std::cout << "Skipping sender: " << sender->getNick() << std::endl;
        } else {
            std::cout << "Null user encountered in channel" << std::endl;
        }
    }
}
