#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include "User.hpp"
#include <algorithm>
#include <iostream>
#include <set>

class User;

class Channel 
{
    private:
        std::string name;
        std::string topic;
        std::map<std::string, User*> users;
        std::map<std::string, bool> operators;
        std::set<std::string> invitedUsers;
        bool topicRestricted;
        bool inviteOnly;
        std::string key;
        size_t userLimit;

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
        void addInvitedUser(const std::string& nickname);
        bool isInvited(const std::string& nickname) const;
        void setTopicRestricted(bool restricted);
        bool isTopicRestricted() const;
        void setInviteOnly(bool inviteOnly);
        bool isInviteOnly() const;
        void setKey(const std::string& newKey);
        bool checkKey(const std::string& providedKey) const;
        void setUserLimit(size_t limit);
        size_t getUserLimit() const;
        bool isAtCapacity() const;
        std::string getModes() const;
};

#endif