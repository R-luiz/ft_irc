
#include "Server.hpp"

Server::Server()
{
	serSocketFd = -1;
}

Server::~Server() {
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        delete it->second;
    }
    channels.clear();
    closeFds();
    clients.clear();
    fds.clear();
}

void Server::clearClients(int fd) {
    for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); ++it) {
        if (it->fd == fd) {
            fds.erase(it);
            break;
        }
    }
    
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->getFd() == fd) {
            clients.erase(it);
            break;
        }
    }
}

void Server::closeFds(){ //-> close the file descriptors
	for(size_t i = 0; i < clients.size(); i++)//-> close the file descriptors of the clients
		close(clients[i].getFd());
	if (serSocketFd != -1) //-> close the server socket file descriptor
		close(serSocketFd);
}

bool Server::Signal = false; //-> initialize the static boolean
void Server::signalHandler(int signum) //-> signal handler
{ 
	std::cout << RED << "Signal " << signum << " received" << WHI << std::endl;
	Signal = true;
}

void Server::serSocket()
{
	struct sockaddr_in add;
	struct pollfd NewPoll;
	add.sin_family = AF_INET; //-> set the address family to ipv4
	add.sin_port = htons(this->Port); //-> convert the port to network byte order (big endian)
	add.sin_addr.s_addr = INADDR_ANY; //-> set the address to any local machine address

	serSocketFd = socket(AF_INET, SOCK_STREAM, 0); //-> create the server socket
	if(serSocketFd == -1) //-> check if the socket is created
		throw(std::runtime_error("failed to create socket"));

	int en = 1;
	if(setsockopt(serSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) //-> set the socket option (SO_REUSEADDR) to reuse the address
		throw(std::runtime_error("failed to set option (SO_REUSEADDR) on socket"));
	if (fcntl(serSocketFd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		throw(std::runtime_error("failed to set option (O_NONBLOCK) on socket"));
	if (bind(serSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) //-> bind the socket to the address
		throw(std::runtime_error("failed to bind socket"));
	if (listen(serSocketFd, SOMAXCONN) == -1) //-> listen for incoming connections and making the socket a passive socket
		throw(std::runtime_error("listen() failed"));

	NewPoll.fd = serSocketFd; //-> add the server socket to the pollfd
	NewPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	NewPoll.revents = 0; //-> set the revents to 0
	fds.push_back(NewPoll); //-> add the server socket to the pollfd
}

void Server::acceptNewClient()
{
    struct sockaddr_in cliadd;
    socklen_t len = sizeof(cliadd);

    int incofd = accept(serSocketFd, (sockaddr *)&(cliadd), &len);
    if (incofd == -1)
    {
        std::cerr << "accept() failed" << std::endl;
        return;
    }

    if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl() failed" << std::endl;
        close(incofd);
        return;
    }

    struct pollfd NewPoll;
    NewPoll.fd = incofd;
    NewPoll.events = POLLIN;
    NewPoll.revents = 0;

    Client cli;
    cli.setFd(incofd);
    cli.setIpAdd(inet_ntoa(cliadd.sin_addr));
    cli.getUser()->setFd(incofd);
    clients.push_back(cli);
    fds.push_back(NewPoll);

    std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::printUserParts(User user) {
	std::cout << "--> User parts:" << std::endl;
	std::cout << "--> Nick: " << user.getNick() << std::endl;
	std::cout << "--> Username: " << user.getUser() << std::endl;
	std::cout << "--> Hostname: " << user.getHostname() << std::endl;
	std::cout << "--> Fd: " << user.getFd() << std::endl;
	std::cout << "--> End of user parts" << std::endl;
}

void Server::auth(int fd, std::string pass) {
    Client* client = getClientByFd(fd);
    if (!client) {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }
    
    if (pass == this->Pass) {
        client->setAuthenticated(true);
        std::string response = "001 :Password accepted\r\n";
        send(fd, response.c_str(), response.length(), 0);
    } else {
        std::string error = "464 :Password incorrect\r\n";
        send(fd, error.c_str(), error.length(), 0);
        close(fd);
    }
}

void Server::setClientUsername(int fd, const std::string& username, const std::string& hostname, const std::string& realname) {
    Client* client = getClientByFd(fd);
    if (!client) {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }

    User* user = client->getUser();
    if (!user) {
        std::cerr << "Error: User object not found for client." << std::endl;
        return;
    }

    if (!username.empty())
        user->setUser(username);
    if (!hostname.empty())
        user->setHostname(hostname);
    if (!realname.empty())
        user->setRealName(realname);

    if (!user->getNick().empty() && !user->getUser().empty()) {
        if (!client->isAuthenticated()) {
            client->setAuthenticated(true);
            sendWelcomeMessages(fd);
        }
    }

    std::string response = ":server 001 " + user->getNick() + " :Welcome to the IRC Network " + 
                           user->getNick() + "!" + user->getUser() + "@" + user->getHostname() + "\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

bool Server::isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > 9) {
        return false;
    }
    for (size_t i = 0; i < nick.length(); ++i) {
        char c = nick[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^' || c == '{' || c == '}')) {
            return false;
        }
    }
    return true;
}

void Server::handlePrivmsg(int senderFd, const std::string& target, const std::string& message)
{
    std::cout << "--1---->Handling PRIVMSG from client <" << senderFd << ">" << std::endl;
    Client* sender = getClientByFd(senderFd);
    if (!sender || !sender->getUser()) {
        std::cerr << "Error: Invalid sender in handlePrivmsg" << std::endl;
        return;
    }
    
    if (target[0] == '#') {
        std::cout << "--2------>Channel message" << std::endl;
        Channel* channel = getChannel(target);
        if (channel)
        {
            std::cout << "--3------>Channel message" << std::endl;
            User* senderUser = sender->getUser();
            if (senderUser && channel->hasUser(senderUser))
            {
                std::cout << "--4------>Channel message" << std::endl;
                std::string fullMessage;
                try {
                    fullMessage = ":" + senderUser->getNick() + "!" +
                                  senderUser->getUser() + "@" +
                                  senderUser->getHostname() +
                                  " PRIVMSG " + target + " :" + message + "\r\n";
                } catch (const std::exception& e) {
                    std::cerr << "Error creating message string: " << e.what() << std::endl;
                    return;
                }
                channel->broadcastMessage(fullMessage, senderUser);
            }
            else
            {
                std::cout << "--5------>Channel message" << std::endl;
                std::string error = ":server 404 " + senderUser->getNick() + " " + target + " :Cannot send to channel\r\n";
                send(senderFd, error.c_str(), error.length(), 0);
            }
        } else {
            std::string error = ":server 403 " + sender->getUser()->getNick() + " " + target + " :No such channel\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
        }
    } else {
        // Private message
        if (target.empty() || !isValidNickname(target)) {
            std::string error = ":server 401 " + sender->getUser()->getNick() + " " + target + " :Invalid nickname\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
            return;
        }

        if (sender->getUser()->getNick() == target) {
            std::string error = ":server 400 " + sender->getUser()->getNick() + " :Cannot send message to yourself\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
            return;
        }

        Client* recipient = NULL;
        for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
            if (it->getUser()->getNick() == target) {
                recipient = &(*it);
                break;
            }
        }

        if (recipient) {
            std::string fullMessage = ":" + sender->getUser()->getNick() + "!" +
                                      sender->getUser()->getUser() + "@" +
                                      sender->getUser()->getHostname() +
                                      " PRIVMSG " + target + " :" + message + "\r\n";
            ssize_t sent = send(recipient->getFd(), fullMessage.c_str(), fullMessage.length(), 0);
            if (sent == -1) {
                std::cerr << "Error sending private message to " << target << ": " << strerror(errno) << std::endl;
                std::string error = ":server 401 " + sender->getUser()->getNick() + " " + target + " :Failed to send message\r\n";
                send(senderFd, error.c_str(), error.length(), 0);
            } else if (static_cast<size_t>(sent) < fullMessage.length()) {
                std::cerr << "Incomplete private message sent to " << target << std::endl;
            } else {
                // Log the private message (optional)
                std::cout << "Private message sent from " << sender->getUser()->getNick() << " to " << target << std::endl;
            }
        } else {
            std::string error = ":server 401 " + sender->getUser()->getNick() + " " + target + " :No such nick\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
        }
    }
}

void Server::processClientInput(const char *buff, int fd)
{
    std::string input(buff);
    std::istringstream iss(input);
    std::string command;
	std::cout << "received: " << input << " - from client <" << fd << ">" << std::endl;
    Client* client = getClientByFd(fd);

    if (!client) {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }
    if (!(iss >> command)) {
        std::cerr << "Empty command received from client <" << fd << ">" << std::endl;
        return;
    }
    if (command == "CAP") {
        std::string capCommand;
        iss >> capCommand;
        if (capCommand == "LS")
            send(fd, "CAP * LS :\r\n", 12, 0);
        else if (capCommand == "END")
            send(fd, "CAP * ACK :multi-prefix\r\n", 25, 0);
    }
    else if (command == "PASS") {
        std::string pass;
        iss >> pass;
        Server::auth(fd, pass);
    }
    else if (!client->isAuthenticated()) {
        std::string error = "451 :You have not registered\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }
    else if (command == "NICK") {
        std::string nick;
        if (iss >> nick) {
            setClientNickname(fd, nick);
        } else {
            std::string error = "431 :No nickname given\r\n";
            send(fd, error.c_str(), error.length(), 0);
        }
    }
    else if (command == "USER") {
        std::string username, hostname, servername, realname;
        iss >> username >> hostname >> servername;
        std::getline(iss >> std::ws, realname);
        if (!realname.empty() && realname[0] == ':') {
            realname = realname.substr(1);
        }
        setClientUsername(fd, username, hostname, realname);
    }
    else if (command == "userhost") {
        std::string username;
        iss >> username;
        setClientUsername(fd, username, "", "");
    }
    else if (command == "MODE") {
        std::string channel, mode;
        iss >> channel >> mode;
        handleMode(fd, channel, mode);
    }
    else if (command == "WHOIS") {
        std::string target;
        iss >> target;
        handleWhois(fd, target);
    }
    else if (command == "PING") {
        std::string server;
        iss >> server;
        handlePing(fd, server);
    }
    else if (command == "JOIN") {
        std::string channelName;
        iss >> channelName;
        handleJoin(fd, channelName);
    }
    else if (command == "PRIVMSG") {
        std::string target, message;
        iss >> target;
        std::getline(iss >> std::ws, message);
        if (!message.empty() && message[0] == ':') {
            message = message.substr(1);
        }
        handlePrivmsg(fd, target, message);
    }
    else if (command == "KICK")
    {
        std::string channelName, targetNick, reason;
        iss >> channelName >> targetNick;
        std::getline(iss >> std::ws, reason);
        if (reason.empty())
            reason = targetNick;
        else if (reason[0] == ':')
            reason = reason.substr(1);
        handleKick(fd, channelName, targetNick, reason);
    }
    else if (command == "PART") {
        std::string channelName, reason;
        iss >> channelName;
        std::getline(iss >> std::ws, reason);
        if (reason.empty())
            reason = client->getUser()->getNick();
        else if (reason[0] == ':')
            reason = reason.substr(1);
        handlePart(fd, channelName, reason);
    }
    else {
        std::string error = "421 * " + command + " :Unknown command\r\n";
        send(fd, error.c_str(), error.length(), 0);
    }
}

Client* Server::getClientByNick(const std::string& nick) {
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->getUser()->getNick() == nick) {
            return &(*it);
        }
    }
    return NULL;
}
void Server::handlePart(int fd, const std::string& channelName, const std::string& reason) {
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser()) {
        std::cerr << "Invalid client in handlePart" << std::endl;
        return;
    }

    Channel* channel = getChannel(channelName);
    if (!channel) {
        std::string error = ":server 403 " + client->getUser()->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->hasUser(client->getUser())) {
        std::string error = ":server 442 " + client->getUser()->getNick() + " " + channelName + " :You're not on that channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string partMessage = ":" + client->getUser()->getNick() + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() +
                              " PART " + channelName + " :" + reason + "\r\n";
    
    channel->broadcastMessage(partMessage, NULL);
    channel->removeUser(client->getUser());

    // Send the PART message to the leaving user as well
    send(fd, partMessage.c_str(), partMessage.length(), 0);
}

void Server::handleKick(int fd, const std::string& channelName, const std::string& targetNick, const std::string& reason) {
    Client* kicker = getClientByFd(fd);
    if (!kicker || !kicker->getUser()) {
        std::cerr << "Invalid kicker in handleKick" << std::endl;
        return;
    }

    Channel* channel = getChannel(channelName);
    if (!channel) {
        std::string error = ":server 403 " + kicker->getUser()->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(kicker->getUser())) {
        std::string error = ":server 482 " + kicker->getUser()->getNick() + " " + channelName + " :You're not channel operator\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    Client* target = getClientByNick(targetNick);
    if (!target || !channel->hasUser(target->getUser())) {
        std::string error = ":server 441 " + kicker->getUser()->getNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    // Prepare and send the KICK message
    std::string kickMessage = ":" + kicker->getUser()->getNick() + "!" + kicker->getUser()->getUser() + "@" + kicker->getUser()->getHostname() +
                              " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    channel->broadcastMessage(kickMessage, NULL);

    // Prepare and send the PART message
    std::string partMessage = ":" + targetNick + "!" + target->getUser()->getUser() + "@" + target->getUser()->getHostname() +
                              " PART " + channelName + " :Kicked by " + kicker->getUser()->getNick() + "\r\n";
    channel->broadcastMessage(partMessage, NULL);

    // Remove the user from the channel
    channel->removeUser(target->getUser());

    // Send both KICK and PART messages to the kicked user
    send(target->getFd(), kickMessage.c_str(), kickMessage.length(), 0);
    send(target->getFd(), partMessage.c_str(), partMessage.length(), 0);
}

bool Server::isNickInUse(const std::string& nick) {
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->getUser() && it->getUser()->getNick() == nick) {
            return true;
        }
    }
    return false;
}

void Server::setClientNickname(int fd, std::string& nick) {
    Client* client = getClientByFd(fd);
    if (!client) {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }

    if (nick.empty()) {
        std::string error = "431 :No nickname given\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    bool isInUse = isNickInUse(nick);

    std::string oldNick = client->getUser()->getNick();
    if (isInUse)
        nick = nick + "_2";
    client->getUser()->setNick(nick);
    client->setNickSet(true);

    std::string response;
    if (oldNick.empty()) {
        response = ":" + nick + " NICK :" + nick + "\r\n";
    } else {
        response = ":" + oldNick + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() + " NICK :" + nick + "\r\n";
    }
    send(fd, response.c_str(), response.length(), 0);

    checkRegistration(client);
}

void Server::sendWelcomeMessages(int fd) {
    Client* client = getClientByFd(fd);
    if (!client) return;

    User* user = client->getUser();
    std::string nick = user->getNick();
    std::string username = user->getUser();
    std::string hostname = user->getHostname();

    std::string welcome = ":server 001 " + nick + " :Welcome to the IRC Network " + nick + "!" + username + "@" + hostname + "\r\n";
    send(fd, welcome.c_str(), welcome.length(), 0);

    // Send other welcome messages (002, 003, 004, etc.)
    std::string yourHost = ":server 002 " + nick + " :Your host is server, running version 1.0\r\n";
    std::string created = ":server 003 " + nick + " :This server was created " + __DATE__ + " " + __TIME__ + "\r\n";
    std::string myInfo = ":server 004 " + nick + " server 1.0 o o\r\n";

    send(fd, yourHost.c_str(), yourHost.length(), 0);
    send(fd, created.c_str(), created.length(), 0);
    send(fd, myInfo.c_str(), myInfo.length(), 0);
}

void Server::handleMode(int fd, const std::string& channel, const std::string& mode) {
    // Implement channel mode handling
    std::string response = ":server 324 " + channel + " " + mode + "\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

void Server::handleWhois(int fd, const std::string& target) {
    // Implement WHOIS command
    std::string response = ":server 311 " + target + " " + target + " username hostname * :realname\r\n";
    response += ":server 318 " + target + " :End of /WHOIS list.\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

void Server::handlePing(int fd, const std::string& server) {
    std::string response = "PONG :" + server + "\r\n";
    ssize_t sent = send(fd, response.c_str(), response.length(), 0);
    if (sent == -1) {
        // Handle send error
        std::cerr << "Error sending PONG response" << std::endl;
    } else if (static_cast<size_t>(sent) < response.length()) {
        // Handle partial send
        std::cerr << "Partial PONG response sent" << std::endl;
    }
}

Client* Server::getClientByFd(int fd) {
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->getFd() == fd) {
            return &(*it);
        }
    }
    return NULL;
}

void Server::disconnectClient(int fd) {
    std::vector<struct pollfd>::iterator it;
    for (it = fds.begin(); it != fds.end(); ++it) {
        if (it->fd == fd) {
            fds.erase(it);
            break;
        }
    }

    std::vector<Client>::iterator client_it;
    for (client_it = clients.begin(); client_it != clients.end(); ++client_it) {
        if (client_it->getFd() == fd) {
            // Remove the client from all channels
            std::map<std::string, Channel*>::iterator channel_it;
            for (channel_it = channels.begin(); channel_it != channels.end(); ++channel_it) {
                channel_it->second->removeUser(client_it->getUser());
            }
            clients.erase(client_it);
            break;
        }
    }

    close(fd);
}

void Server::checkRegistration(Client* client) {
    if (client && client->isNickSet() && client->isUserSet() && !client->isAuthenticated()) {
        client->setAuthenticated(true);
        sendWelcomeMessages(client->getFd());
    }
}

void Server::receiveNewData(int fd)
{
    char temp_buff[1024];
    ssize_t bytes = recv(fd, temp_buff, sizeof(temp_buff) - 1, 0);
    if (bytes <= 0)
    {
        if (bytes == 0) {
            std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
        } else {
            std::cerr << "Error receiving data from client <" << fd << ">" << std::endl;
        }
        disconnectClient(fd);
    }
    else
    {
        temp_buff[bytes] = '\0';
        Client* client = getClientByFd(fd);
        if (client)
        {
            client->appendToBuffer(temp_buff);
            
            size_t pos;
            while ((pos = client->buffer.find("\r\n")) != std::string::npos)
            {
                std::string command = client->buffer.substr(0, pos);
                processClientInput(command.c_str(), fd);
                client->buffer.erase(0, pos + 2);
            }
        }
        else
        {
            std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        }
    }
}
void Server::printServerState() {
    static int updateCount = 0;
    updateCount++;

    std::ofstream outFile("server_state.log", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open server_state.log for writing." << std::endl;
        return;
    }

    outFile << "==== Server State (Update #" << updateCount << ") ====" << std::endl;
    outFile << "Server Socket FD: " << serSocketFd << std::endl;
    outFile << "Port: " << Port << std::endl;
    outFile << "Number of clients: " << clients.size() << std::endl;
    outFile << std::endl;
    outFile << "=== Clients ===" << std::endl;
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client& client = *it;
        outFile << "Client FD: " << client.getFd() << std::endl;
        outFile << "IP Address: " << client.getIPadd() << std::endl;
        outFile << "Authenticated: " << (client.isAuthenticated() ? "Yes" : "No") << std::endl;
        
        User* user = client.getUser();
        if (user) {
            outFile << "  Nickname: " << user->getNick() << std::endl;
            outFile << "  Username: " << user->getUser() << std::endl;
            outFile << "  Hostname: " << user->getHostname() << std::endl;
            outFile << "  Realname: " << user->getRealName() << std::endl;
        } else {
            outFile << "  User data not available" << std::endl;
        }
        outFile << "  Buffer: " << (client.getBuffer().empty() ? "Empty" : client.getBuffer()) << std::endl;
        outFile << std::endl;
    }
    outFile << "=== Active File Descriptors ===" << std::endl;
    for (std::vector<struct pollfd>::const_iterator it = fds.begin(); it != fds.end(); ++it) {
        outFile << "FD: " << it->fd << ", Events: " << it->events << ", Revents: " << it->revents << std::endl;
    }
    outFile << std::endl;
    outFile << "Server is running. Check console for stop instructions." << std::endl;

    outFile.close();
}

void Server::serverInit(int port, std::string pass)
{
    this->Port = port;
    this->Pass = pass;
    this->serSocket();

    std::cout << GRE << "Server <" << serSocketFd << "> Connected" << WHI << std::endl;
    std::cout << "Waiting to accept a connection...\n";
    while (Server::Signal == false)
    {
        int poll_result = poll(&fds[0], fds.size(), -1);
        if (poll_result == -1 && Server::Signal == false)
        {
            throw(std::runtime_error("poll() failed"));
        }
        else if (poll_result > 0)
        {
            for (size_t i = 0; i < fds.size(); ++i)
            {
                if (fds[i].revents & POLLIN)
                {
                    if (fds[i].fd == serSocketFd)
                    {
                        acceptNewClient();
                    }
                    else
                    {
                        receiveNewData(fds[i].fd);
                    }
                }
                else if (fds[i].revents & (POLLHUP | POLLERR))
                {
                    std::cout << RED << "Client <" << fds[i].fd << "> Disconnected" << WHI << std::endl;
                    disconnectClient(fds[i].fd);
                    --i; // Decrement i because we removed an element from fds
                }
            }
        }
    }
}

Channel *Server::getChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        std::cerr << "Error: Channel " << channelName << " not found." << std::endl;
        return NULL;
    }
    return it->second;
}

void Server::handleChannelMessage(int senderFd, const std::string& channelName, const std::string& message) {
    Client* client = getClientByFd(senderFd);
    if (!client || !client->getUser()) return;

    Channel* channel = getChannel(channelName);
    if (!channel) return;

    User* sender = client->getUser();
    std::string formattedMessage = ":" + sender->getNick() + "!" + sender->getUser() + "@" + sender->getHostname() + " PRIVMSG " + channelName + " :" + message + "\r\n";

    channel->broadcastMessage(formattedMessage, sender);
}

void Server::handleJoin(int fd, const std::string& channelName) {
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser()) {
        std::cerr << "Invalid client in handleJoin" << std::endl;
        return;
    }

    Channel* channel = getOrCreateChannel(channelName);
    if (!channel) {
        std::cerr << "Failed to create or get channel " << channelName << std::endl;
        return;
    }

    User* user = client->getUser();
    if (channel->hasUser(user)) {
        std::string error = ":server 443 " + user->getNick() + " " + channelName + " :is already on channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    bool isFirstUser = channel->getUsers().empty();
    channel->addUser(user, isFirstUser);

    std::string joinMessage = ":" + user->getNick() + "!" + user->getUser() + "@" + user->getHostname() + " JOIN :" + channelName + "\r\n";
    send(fd, joinMessage.c_str(), joinMessage.length(), 0);

    std::string topicMessage = ":server 332 " + user->getNick() + " " + channelName + " :" + channel->getTopic() + "\r\n";
    send(fd, topicMessage.c_str(), topicMessage.length(), 0);

    sendChannelUserList(fd, channel);
    channel->broadcastMessage(joinMessage, user);
}

Channel* Server::getOrCreateChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        Channel* newChannel = new Channel(channelName);
        channels[channelName] = newChannel;
        return newChannel;
    }
    return it->second;
}

void Server::sendChannelUserList(int fd, Channel* channel) {
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser() || !channel) {
        std::cerr << "Invalid client or channel in sendChannelUserList" << std::endl;
        return;
    }

    try {
        std::string userList = ":server 353 " + client->getUser()->getNick() + " = " + channel->getName() + " :";
        std::vector<User*> users = channel->getUsers();
        
        for (std::vector<User*>::const_iterator it = users.begin(); it != users.end(); ++it) {
            if (*it) {
                if (channel->isOperator(*it)) {
                    userList += "@";
                }
                userList += (*it)->getNick() + " ";
            }
        }
        userList += "\r\n";

        if (send(fd, userList.c_str(), userList.length(), 0) == -1) {
            std::cerr << "Error sending user list to client" << std::endl;
        }

        std::string endOfList = ":server 366 " + client->getUser()->getNick() + " " + channel->getName() + " :End of /NAMES list.\r\n";
        if (send(fd, endOfList.c_str(), endOfList.length(), 0) == -1) {
            std::cerr << "Error sending end of list to client" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in sendChannelUserList: " << e.what() << std::endl;
    }
}

bool isPortValid(std::string port)
{
	return (port.find_first_not_of("0123456789") == std::string::npos && \
	std::atoi(port.c_str()) >= 1024 && std::atoi(port.c_str()) <= 49152);
}

int main(int argc, char *argv[])
{
	if (argc != 3 || !isPortValid(argv[1])) //-> check the number of arguments and the port
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}
	Server ser;
	std::cout << "---- SERVER ----" << std::endl;
	int port = std::atoi(argv[1]); //-> convert the port to integer
	std::string pass = argv[2]; //-> get the password
	try{
		signal(SIGINT, Server::signalHandler); //-> catch the signal (ctrl + c)
		signal(SIGQUIT, Server::signalHandler); //-> catch the signal (ctrl + \)
		ser.serverInit(port, pass); //-> initialize the server
	}
	catch(const std::exception& e){
		ser.closeFds(); //-> close the file descriptors
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}