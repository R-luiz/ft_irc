#include "Message.hpp"

Message::Message(Client* recipient, const std::string& content)
    : recipient(recipient), content(content) {}

Client* Message::getRecipient() const {
    return recipient;
}

const std::string& Message::getContent() const {
    return content;
}