#include "Channel.hpp"
#include <algorithm>
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

Channel::Channel(const std::string& name) : name(name) {}

const std::string& Channel::getName() const { return name; }
const std::string& Channel::getTopic() const { return topic; }
void Channel::setTopic(const std::string& newTopic) { topic = newTopic; }

void Channel::addClient(Client* client) {
    clients.push_back(client);
}

void Channel::removeClient(Client* client) {
    std::vector<Client*>::iterator it = std::find(clients.begin(), clients.end(), client);
    if (it != clients.end()) {
        clients.erase(it);
    }
    removeOperator(client);
}

bool Channel::hasClient(Client* client) const {
    return std::find(clients.begin(), clients.end(), client) != clients.end();
}

void Channel::addOperator(Client* client) {
    if (hasClient(client) && !isOperator(client)) {
        operators.push_back(client);
    }
}

void Channel::removeOperator(Client* client) {
    std::vector<Client*>::iterator it = std::find(operators.begin(), operators.end(), client);
    if (it != operators.end()) {
        operators.erase(it);
    }
}

bool Channel::isOperator(Client* client) const {
    return std::find(operators.begin(), operators.end(), client) != operators.end();
}

void Channel::broadcast(const std::string& message, Client* exclude) {
    for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (*it != exclude) {
            (*it)->sendMessage(message);
        }
    }
}

const std::vector<Client*>& Channel::getClients() const {
    return clients;
}
bool Channel::isInviteOnly() const {
    return inviteOnly;
}

void Channel::setInviteOnly(bool value) {
    inviteOnly = value;
}

bool Channel::isTopicRestricted() const {
    return topicRestricted;
}

void Channel::setTopicRestricted(bool value) {
    topicRestricted = value;
}

void Channel::setKey(const std::string& newKey) {
    key = newKey;
}

void Channel::removeKey() {
    key.clear();
}

const std::string& Channel::getKey() const {
    return key;
}

void Channel::setUserLimit(int limit) {
    userLimit = limit;
}

void Channel::removeUserLimit() {
    userLimit = -1;
}

int Channel::getUserLimit() const {
    return userLimit;
}

void Channel::addInvitedUser(Client* client) {
    invitedUsers.push_back(client);
}

bool Channel::isInvited(Client* client) const {
    return std::find(invitedUsers.begin(), invitedUsers.end(), client) != invitedUsers.end();
}

std::string Channel::getModes() const {
    std::ostringstream modes;
    if (inviteOnly) modes << "i";
    if (topicRestricted) modes << "t";
    if (!key.empty()) modes << "k";
    if (userLimit != -1) modes << "l";
    return modes.str();
}