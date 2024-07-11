#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

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
};

#endif