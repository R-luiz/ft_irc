#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include "Channel.hpp"
#include "Message.hpp"
#include "MessageQueue.hpp"

class Client;
class Channel;

class Server {
private:
    int port;
    std::string password;
    int serverSocket;
    std::vector<pollfd> fds;
    std::vector<Client*> clients;
    std::vector<Channel*> channels;

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();

private:
    void setup();
    void handleNewConnection();
    void handleClientMessage(int clientFd);
    void removeClient(int clientFd);
    void processMessage(int clientFd, const std::string& message);
	void handleCommand(Client* client, const std::string& command, const std::string& params);
    void handleNick(Client* client, const std::string& nickname);
    void handleUser(Client* client, const std::string& params);
    void handlePing(Client* client, const std::string& token);
    void authenticateClient(Client* client);
	Client* getClientByFd(int fd);
	void handleJoin(Client* client, const std::string& params);
    void handlePrivmsg(Client* client, const std::string& params);
    Channel* getChannelByName(const std::string& name);
	Client* getClientByNickname(const std::string& nickname);
	void handlePart(Client* client, const std::string& params);
    void handleKick(Client* client, const std::string& params);
    void handleInvite(Client* client, const std::string& params);
    void handleTopic(Client* client, const std::string& params);
    void handleMode(Client* client, const std::string& params);
    void handlePass(Client* client, const std::string& params);
	MessageQueue messageQueue;
    void processMessageQueue();
    void queueMessage(Client* recipient, const std::string& content);
    void broadcastServerMessage(const std::string& message);
    std::string getServerName() const;
    std::string getServerCreationDate() const;
};

#endif