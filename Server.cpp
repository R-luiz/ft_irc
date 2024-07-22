
#include "Server.hpp"

Server::Server(int port, const std::string& password) 
    : serverSocket(-1), port(port), password(password) {
    setup();
}

Server::~Server() {
	close(serverSocket);
}

void Server::setup() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket, 3) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    // Set server socket to non-blocking
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    flags |= O_NONBLOCK;
    if (fcntl(serverSocket, F_SETFL, flags) == -1) {
        throw std::runtime_error("Failed to set socket to non-blocking");
    }

    pollfd serverPollFd = {serverSocket, POLLIN, 0};
    fds.push_back(serverPollFd);
}
