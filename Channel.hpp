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
        std::map<std::string, User*> users;
        std::map<std::string, bool> operators; // true if the user is an operator

    public:
        Channel(const std::string& channelName);
        ~Channel();

        void addUser(User* user, bool isOperator);
        void removeUser(const std::string& nickname);
        bool hasUser(const std::string& nickname) const;
        void setTopic(const std::string& newTopic);
        std::string getTopic() const;
        std::string getName() const;
        std::map<std::string, User*> getUsers() const;
        bool isOperator(const std::string& nickname) const;
        void setOperator(const std::string& nickname, bool status);
        void broadcastMessage(const std::string& message, User* sender);
};

#endif