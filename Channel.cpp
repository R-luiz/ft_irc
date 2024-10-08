#include "Channel.hpp"

Channel::Channel(const std::string& channelName) 
    : name(channelName), topic(""), topicRestricted(false), inviteOnly(false), key(""), userLimit(0) {}

Channel::~Channel()
{
    users.clear();
    operators.clear();
}

void Channel::addUser(User* user, bool isOperator)
{
    if (user && !user->getNick().empty())
    {
        std::string nick = user->getNick();
        users[nick] = user;
        operators[nick] = isOperator;
        std::cout << "Added user " << nick << " to channel " << name << std::endl;
        
        // Notify other users in the channel
        std::string joinMessage = ":" + nick + "!" + user->getUser() + "@" + user->getHostname() + " JOIN :" + name + "\r\n";
        broadcastMessage(joinMessage, user);
    }
    else
        std::cerr << "Attempted to add invalid user to channel " << name << std::endl;
}

void Channel::removeUser(const std::string& nickname)
{
    std::cout << "Removing user " << nickname << " from channel " << name << std::endl;
    std::map<std::string, User*>::iterator it = users.find(nickname);
    if (it != users.end()) {
        User* user = it->second;
        users.erase(it);
        operators.erase(nickname);
        std::cout << "User " << nickname << " removed from channel " << name << std::endl;
        
        // Notify other users in the channel
        std::string partMessage = ":" + nickname + " PART " + name + " :User disconnected\r\n";
        broadcastMessage(partMessage, user);
    } else {
        std::cout << "User " << nickname << " not found in channel " << name << std::endl;
    }
}

bool Channel::hasUser(const std::string& nickname) const
{
    std::cout << "Checking if user " << nickname << " is in channel " << name << std::endl;
    bool found = users.find(nickname) != users.end();
    std::cout << "User " << nickname << " is " << (found ? "" : "not ") << "in channel " << name << std::endl;
    return found;
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
    std::cout << "Broadcasting message in channel " << name << ": " << message << std::endl;
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
                    std::cout << "Message content: " << message << std::endl;
                    ssize_t sent = send(userFd, message.c_str(), message.length(), 0);
                    if (sent == -1) {
                        std::cerr << "Error sending message to user " << nick << ": " << strerror(errno) << std::endl;
                    } else if (static_cast<size_t>(sent) < message.length()) {
                        std::cerr << "Partial message sent to user " << nick << ". Sent " << sent << " of " << message.length() << " bytes." << std::endl;
                    } else {
                        std::cout << "Message sent successfully to user " << nick << ". Sent " << sent << " bytes." << std::endl;
                    }
                } else {
                    std::cerr << "Invalid fd or empty message for user " << nick << std::endl;
                }
            } catch (const std::bad_alloc& e) {
                std::cerr << "Memory allocation failed while sending message to " << it->first << ": " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Exception caught while sending message to " << it->first << ": " << e.what() << std::endl;
                std::cerr << "User details - Nick: " << user->getNick() << ", FD: " << user->getFd() << std::endl;
            }
        }
    }
}

void Channel::addInvitedUser(const std::string& nickname) {
    invitedUsers.insert(nickname);
}

bool Channel::isInvited(const std::string& nickname) const {
    return invitedUsers.find(nickname) != invitedUsers.end();
}

void Channel::setTopicRestricted(bool restricted) {
    topicRestricted = restricted;
}

bool Channel::isTopicRestricted() const {
    return topicRestricted;
}

std::string Channel::getModes() const {
    std::string modes = "+";
    if (inviteOnly) modes += "i";
    if (topicRestricted) modes += "t";
    if (!key.empty()) modes += "k";
    if (userLimit > 0) modes += "l";
    return modes;
}

void Channel::setInviteOnly(bool inviteOnly) {
    std::cout << "Setting invite-only mode to " << (inviteOnly ? "true" : "false") << " for channel " << name << std::endl;
    this->inviteOnly = inviteOnly;
}

void Channel::setKey(const std::string& newKey) {
    std::cout << "Setting key to '" << newKey << "' for channel " << name << std::endl;
    this->key = newKey;
}

void Channel::setUserLimit(size_t limit) {
    std::cout << "Setting user limit to " << limit << " for channel " << name << std::endl;
    this->userLimit = limit;
}

bool Channel::isInviteOnly() const {
    return inviteOnly;
}

bool Channel::checkKey(const std::string& providedKey) const {
    return key.empty() || key == providedKey;
}

size_t Channel::getUserLimit() const {
    return userLimit;
}

bool Channel::isAtCapacity() const {
    return userLimit > 0 && users.size() >= userLimit;
}
