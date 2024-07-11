// MessageQueue.cpp
#include "MessageQueue.hpp"

void MessageQueue::push(const Message& msg) {
    queue.push_back(msg);
}

Message MessageQueue::pop() {
    if (queue.empty()) {
        throw std::runtime_error("Attempt to pop from an empty queue");
    }
    Message msg = queue.front();
    queue.pop_front();
    return msg;
}

bool MessageQueue::empty() const {
    return queue.empty();
}