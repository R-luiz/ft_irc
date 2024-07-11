#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

class Client;

class Message {
private:
    Client* recipient;
    std::string content;

public:
    Message(Client* recipient, const std::string& content);
    Client* getRecipient() const;
    const std::string& getContent() const;
};

#endif