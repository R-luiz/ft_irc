#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
class Client;
class Channel;

class Server {
private:
    int serverSocket;
    int port;
    std::string password;
    std::vector<pollfd> fds;
    std::map<int, Client*> clients;
    std::map<std::string, Channel*> channels;

    void setup();
    void handleNewConnection();
    void handleClientMessage(int clientFd);
    void removeClient(int clientFd);
    void processCommand(Client* client, const std::string& message);

    // Mandatory command handlers
    void handlePass(Client* client, const std::string& params);
    void handleNick(Client* client, const std::string& params);
    void handleUser(Client* client, const std::string& params);
    void handleJoin(Client* client, const std::string& params);
    void handlePrivmsg(Client* client, const std::string& params);
    void handleKick(Client* client, const std::string& params);
    void handleInvite(Client* client, const std::string& params);
    void handleTopic(Client* client, const std::string& params);
    void handleMode(Client* client, const std::string& params);

    Channel* getChannel(const std::string& channelName);
    void broadcastToChannel(const Channel* channel, const std::string& message, Client* exclude = nullptr);

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();
};

#endif // SERVER_HPP