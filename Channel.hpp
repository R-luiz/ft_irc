#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
private:
    std::string name;
    std::string topic;
    std::vector<Client*> clients;
    std::vector<Client*> operators;
    std::vector<Client*> invitedUsers;
    bool inviteOnly;
    bool topicRestricted;
    std::string key;
    int userLimit;

public:
    Channel(const std::string& name);
    
    const std::string& getName() const;
    const std::string& getTopic() const;
    void setTopic(const std::string& newTopic);
    
    void addClient(Client* client);
    void removeClient(Client* client);
    bool hasClient(Client* client) const;
    
    void addOperator(Client* client);
    void removeOperator(Client* client);
    bool isOperator(Client* client) const;
    
    void broadcast(const std::string& message, Client* exclude = NULL);
    
    const std::vector<Client*>& getClients() const;

    // New methods
    bool isInviteOnly() const;
    void setInviteOnly(bool value);
    bool isTopicRestricted() const;
    void setTopicRestricted(bool value);
    void setKey(const std::string& newKey);
    void removeKey();
    const std::string& getKey() const;
    void setUserLimit(int limit);
    void removeUserLimit();
    int getUserLimit() const;
    void addInvitedUser(Client* client);
    bool isInvited(Client* client) const;
    std::string getModes() const;
};

#endif