#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include "User.hpp"
#include <algorithm>
#include <iostream>

class User;

class Channel 
{
    private:
        std::string name;
        std::string topic;
        std::vector<User*> users;
        std::map<User*, bool> operators; // true if the user is an operator

    public:
        Channel(const std::string& channelName);
        ~Channel();
  
    void addUser(User* user, bool isOperator);
    void removeUser(User* user);
    bool hasUser(User* user) const;
    void setTopic(const std::string& newTopic);
    std::string getTopic() const;
    std::string getName() const;
    std::vector<User*> getUsers() const;
    bool isOperator(User* user) const;
    void setOperator(User* user, bool status);
    void broadcastMessage(const std::string& message, User* sender);
};

#endif