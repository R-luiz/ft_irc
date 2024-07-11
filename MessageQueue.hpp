// MessageQueue.hpp
#ifndef MESSAGEQUEUE_HPP
#define MESSAGEQUEUE_HPP

#include <list>
#include "Message.hpp"

class MessageQueue {
private:
    std::list<Message> queue;

public:
    void push(const Message& msg);
    Message pop();
    bool empty() const;
};

#endif