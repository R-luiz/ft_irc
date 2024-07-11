#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "Client.hpp"
#include <sstream>
#include <algorithm>

Server::Server(int port, const std::string& password)
    : port(port), password(password), serverSocket(-1) {}

Server::~Server() {
    if (serverSocket != -1)
        close(serverSocket);
    // Clean up clients and channels
}

void Server::setup() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket to non-blocking
    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

    // Allow reuse of address
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind to port");
    }

    if (listen(serverSocket, 5) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server is listening on port " << port << std::endl;

    // Set up poll for server socket
    pollfd serverPollFd = {serverSocket, POLLIN, 0};
    fds.push_back(serverPollFd);
}

void Server::run() {
    setup();

    while (true) {
        int activity = poll(&fds[0], fds.size(), -1);
        if (activity < 0) {
            throw std::runtime_error("Poll failed");
        }

        if (fds[0].revents & POLLIN) {
            handleNewConnection();
        }

        for (size_t i = 1; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                handleClientMessage(fds[i].fd);
            }
        }
        processMessageQueue();
    }
}

void Server::handleNewConnection() {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientFd = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to accept client connection" << std::endl;
        }
        return;
    }

    // Set client socket to non-blocking
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    // Add new client to our data structures
    Client* newClient = new Client(clientFd);
    clients.push_back(newClient);

    pollfd clientPollFd = {clientFd, POLLIN, 0};
    fds.push_back(clientPollFd);

    std::cout << "New client connected" << std::endl;
}

void Server::handleClientMessage(int clientFd) {
    char buffer[1024];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Client disconnected" << std::endl;
        } else {
            std::cerr << "Error reading from client" << std::endl;
        }
        removeClient(clientFd);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer);

    // Process the received message
    processMessage(clientFd, message);
}

void Server::removeClient(int clientFd) {
    // Remove client from data structures
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i]->getFd() == clientFd) {
            delete clients[i];
            clients.erase(clients.begin() + i);
            break;
        }
    }

    // Remove from fds vector
    for (size_t i = 0; i < fds.size(); ++i) {
        if (fds[i].fd == clientFd) {
            fds.erase(fds.begin() + i);
            break;
        }
    }

    close(clientFd);
}

void Server::processMessage(int clientFd, const std::string& message) {
    // We'll implement IRC protocol handling in the next step
    std::cout << "Received message from client " << clientFd << ": " << message;
}

void Server::handleCommand(Client* client, const std::string& command, const std::string& params) {
    if (command == "NICK") {
        handleNick(client, params);
    } else if (command == "USER") {
        handleUser(client, params);
    } else if (command == "PING") {
        handlePing(client, params);
    } else if (command == "JOIN") {
        handleJoin(client, params);
    } else if (command == "PRIVMSG") {
        handlePrivmsg(client, params);
    } else if (command == "PART") {
        handlePart(client, params);
    } else if (command == "KICK") {
        handleKick(client, params);
    } else if (command == "INVITE") {
        handleInvite(client, params);
    } else if (command == "TOPIC") {
        handleTopic(client, params);
    } else if (command == "MODE") {
        handleMode(client, params);
    } else {
        // Handle other commands or send error for unknown command
        client->sendMessage("421 " + command + " :Unknown command");
    }
}

void Server::handleUser(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string username, mode, unused, realname;
    iss >> username >> mode >> unused;
    std::getline(iss, realname);
    realname = realname.substr(realname.find_first_not_of(" :"));

    if (username.empty() || realname.empty()) {
        client->sendMessage("461 USER :Not enough parameters");
        return;
    }

    client->setUsername(username);
    // You might want to store the realname as well
    authenticateClient(client);
}

void Server::handlePing(Client* client, const std::string& token) {
    client->sendMessage("PONG :" + token);
}

void Server::authenticateClient(Client* client) {
    if (!client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty()) {
        client->setAuthenticated(true);
        client->sendMessage("001 " + client->getNickname() + " :Welcome to the IRC server");
        // Send other welcome messages (002, 003, 004, etc.)
    }
}
void Server::handleNick(Client* client, const std::string& nickname) {
    // Check if nickname is valid and not already in use
    if (nickname.empty() || nickname.find_first_of(" \t\r\n") != std::string::npos) {
        client->sendMessage("432 * " + nickname + " :Erroneous nickname");
        return;
    }

    for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getNickname() == nickname) {
            client->sendMessage("433 * " + nickname + " :Nickname is already in use");
            return;
        }
    }

    std::string oldNick = client->getNickname();
    client->setNickname(nickname);
    client->sendMessage(":" + oldNick + " NICK " + nickname);
    authenticateClient(client);
}

Client* Server::getClientByFd(int fd) {
    for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getFd() == fd) {
            return *it;
        }
    }
    return NULL;
}


void Server::handleJoin(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName;
    iss >> channelName;

    if (channelName.empty() || channelName[0] != '#') {
        client->sendMessage("461 JOIN :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        channel = new Channel(channelName);
        channels.push_back(channel);
    }

    if (!channel->hasClient(client)) {
        channel->addClient(client);
        channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@host JOIN " + channelName);
        
        // Send channel topic
        if (!channel->getTopic().empty()) {
            client->sendMessage("332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic());
        }
        
        // Send list of users in channel
        std::string userList;
        const std::vector<Client*>& channelClients = channel->getClients();
        for (std::vector<Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it) {
            if (!userList.empty()) userList += " ";
            userList += (*it)->getNickname();
        }
        client->sendMessage("353 " + client->getNickname() + " = " + channelName + " :" + userList);
        client->sendMessage("366 " + client->getNickname() + " " + channelName + " :End of /NAMES list.");
    }
}

void Server::handlePrivmsg(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string target, message;
    iss >> target;
    std::getline(iss, message);
    message = message.substr(message.find_first_not_of(" :"));

    if (target.empty() || message.empty()) {
        client->sendMessage("461 PRIVMSG :Not enough parameters");
        return;
    }

    if (target[0] == '#') {
        // Channel message
        Channel* channel = getChannelByName(target);
        if (channel && channel->hasClient(client)) {
            channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@host PRIVMSG " + target + " :" + message, client);
        } else {
            client->sendMessage("404 " + target + " :Cannot send to channel");
        }
    } else {
        // Private message
        Client* recipient = getClientByNickname(target);
        if (recipient) {
            recipient->sendMessage(":" + client->getNickname() + "!" + client->getUsername() + "@host PRIVMSG " + target + " :" + message);
        } else {
            client->sendMessage("401 " + target + " :No such nick/channel");
        }
    }
}

Channel* Server::getChannelByName(const std::string& name) {
    for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if ((*it)->getName() == name) {
            return *it;
        }
    }
    return NULL;
}

Client* Server::getClientByNickname(const std::string& nickname) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getNickname() == nickname) {
            return *it;
        }
    }
    return NULL;
}

void Server::handlePart(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName;
    iss >> channelName;

    if (channelName.empty()) {
        client->sendMessage("461 PART :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (channel && channel->hasClient(client)) {
        channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@host PART " + channelName);
        channel->removeClient(client);
        if (channel->getClients().empty()) {
            // Remove the channel if it's empty
            std::vector<Channel*>::iterator it = std::find(channels.begin(), channels.end(), channel);
            if (it != channels.end()) {
                channels.erase(it);
                delete channel;
            }
        }
    } else {
        client->sendMessage("442 " + channelName + " :You're not on that channel");
    }
}

void Server::handleKick(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName, targetNick, reason;
    iss >> channelName >> targetNick;
    std::getline(iss, reason);
    reason = reason.substr(reason.find_first_not_of(" :"));

    if (channelName.empty() || targetNick.empty()) {
        client->sendMessage("461 KICK :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        client->sendMessage("403 " + channelName + " :No such channel");
        return;
    }

    if (!channel->isOperator(client)) {
        client->sendMessage("482 " + channelName + " :You're not channel operator");
        return;
    }

    Client* target = getClientByNickname(targetNick);
    if (!target || !channel->hasClient(target)) {
        client->sendMessage("441 " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kickMessage = ":" + client->getNickname() + "!" + client->getUsername() + "@host KICK " + channelName + " " + targetNick;
    if (!reason.empty()) {
        kickMessage += " :" + reason;
    }
    channel->broadcast(kickMessage);
    channel->removeClient(target);
}

void Server::handleInvite(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string targetNick, channelName;
    iss >> targetNick >> channelName;

    if (targetNick.empty() || channelName.empty()) {
        client->sendMessage("461 INVITE :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        client->sendMessage("403 " + channelName + " :No such channel");
        return;
    }

    if (!channel->hasClient(client)) {
        client->sendMessage("442 " + channelName + " :You're not on that channel");
        return;
    }

    if (channel->isInviteOnly() && !channel->isOperator(client)) {
        client->sendMessage("482 " + channelName + " :You're not channel operator");
        return;
    }

    Client* target = getClientByNickname(targetNick);
    if (!target) {
        client->sendMessage("401 " + targetNick + " :No such nick/channel");
        return;
    }

    if (channel->hasClient(target)) {
        client->sendMessage("443 " + targetNick + " " + channelName + " :is already on channel");
        return;
    }

    target->sendMessage(":" + client->getNickname() + "!" + client->getUsername() + "@host INVITE " + targetNick + " " + channelName);
    client->sendMessage("341 " + targetNick + " " + channelName);
    channel->addInvitedUser(target);
}

void Server::handleTopic(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName, newTopic;
    iss >> channelName;
    std::getline(iss, newTopic);
    newTopic = newTopic.substr(newTopic.find_first_not_of(" :"));

    if (channelName.empty()) {
        client->sendMessage("461 TOPIC :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        client->sendMessage("403 " + channelName + " :No such channel");
        return;
    }

    if (!channel->hasClient(client)) {
        client->sendMessage("442 " + channelName + " :You're not on that channel");
        return;
    }

    if (newTopic.empty()) {
        // Requesting the topic
        if (channel->getTopic().empty()) {
            client->sendMessage("331 " + channelName + " :No topic is set");
        } else {
            client->sendMessage("332 " + channelName + " :" + channel->getTopic());
        }
    } else {
        // Setting the topic
        if (channel->isTopicRestricted() && !channel->isOperator(client)) {
            client->sendMessage("482 " + channelName + " :You're not channel operator");
            return;
        }
        channel->setTopic(newTopic);
        channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@host TOPIC " + channelName + " :" + newTopic);
    }
}

void Server::handleMode(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName, modes, modeParams;
    iss >> channelName >> modes;
    std::getline(iss, modeParams);
    modeParams = modeParams.substr(modeParams.find_first_not_of(" "));

    if (channelName.empty()) {
        client->sendMessage("461 MODE :Not enough parameters");
        return;
    }

    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        client->sendMessage("403 " + channelName + " :No such channel");
        return;
    }

    if (modes.empty()) {
        // Requesting channel modes
        std::string currentModes = channel->getModes();
        client->sendMessage("324 " + client->getNickname() + " " + channelName + " +" + currentModes);
        return;
    }

    if (!channel->isOperator(client)) {
        client->sendMessage("482 " + channelName + " :You're not channel operator");
        return;
    }

    // Setting modes
    bool adding = true;
    std::string modeChanges;
    std::istringstream modeParamsStream(modeParams);
    std::string modeParam;

    for (std::string::iterator it = modes.begin(); it != modes.end(); ++it) {
        if (*it == '+') {
            adding = true;
            continue;
        }
        if (*it == '-') {
            adding = false;
            continue;
        }

        switch (*it) {
            case 'i': // Invite-only
                channel->setInviteOnly(adding);
                modeChanges += (adding ? "+" : "-") + std::string(1, *it);
                break;
            case 't': // Topic restriction
                channel->setTopicRestricted(adding);
                modeChanges += (adding ? "+" : "-") + std::string(1, *it);
                break;
            case 'k': // Channel key (password)
                if (adding && modeParamsStream >> modeParam) {
                    channel->setKey(modeParam);
                    modeChanges += "+k " + modeParam;
                } else if (!adding) {
                    channel->removeKey();
                    modeChanges += "-k";
                }
                break;
            case 'o': // Channel operator
                if (modeParamsStream >> modeParam) {
                    Client* target = getClientByNickname(modeParam);
                    if (target && channel->hasClient(target)) {
                        if (adding) {
                            channel->addOperator(target);
                        } else {
                            channel->removeOperator(target);
                        }
                        modeChanges += (adding ? "+" : "-") + std::string(1, *it) + " " + modeParam;
                    }
                }
                break;
            case 'l': // User limit
                if (adding && modeParamsStream >> modeParam) {
                    int limit = std::atoi(modeParam.c_str());
                    if (limit > 0) {
                        channel->setUserLimit(limit);
                        modeChanges += "+l " + modeParam;
                    }
                } else if (!adding) {
                    channel->removeUserLimit();
                    modeChanges += "-l";
                }
                break;
            default:
                client->sendMessage("472 " + std::string(1, *it) + " :is unknown mode char to me");
        }
    }

    if (!modeChanges.empty()) {
        channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@host MODE " + channelName + " " + modeChanges);
    }
}

void Server::broadcastServerMessage(const std::string& message) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        queueMessage(*it, message);
    }
}

void Server::processMessageQueue() {
    while (!messageQueue.empty()) {
        Message msg = messageQueue.pop();
        Client* recipient = msg.getRecipient();
        if (recipient && recipient->isConnected()) {
            recipient->sendMessage(msg.getContent());
        }
    }
}

void Server::queueMessage(Client* recipient, const std::string& content) {
    messageQueue.push(Message(recipient, content));
}