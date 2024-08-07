
#include "Server.hpp"

Server::Server()
{
	serSocketFd = -1;
}

Server::~Server()
{
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        delete it->second;
    }
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
                           user->getNick() + "!" + username + "@" + hostname + "\r\n";
    send(fd, response.c_str(), response.length(), 0);
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
    else {
        std::string error = "421 * " + command + " :Unknown command\r\n";
        send(fd, error.c_str(), error.length(), 0);
    }
}

bool Server::isNickInUse(const std::string& nick) {
    for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->getUser() && it->getUser()->getNick() == nick) {
            return true;
        }
    }
    return false;
}

void Server::setClientNickname(int fd, const std::string& nick) {
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

    if (isNickInUse(nick)) {
        std::string error = "433 * " + nick + " :Nickname is already in use\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string oldNick = client->getUser()->getNick();
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

void Server::checkRegistration(Client* client) {
    if (client->isNickSet() && client->isUserSet() && !client->isAuthenticated()) {
        client->setAuthenticated(true);
        sendWelcomeMessages(client->getFd());
    }
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

void Server::disconnectClient(int fd)
{
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
    if (!client) {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }

    User* user = client->getUser();
    if (!user) {
        std::cerr << "Error: User object not found for client." << std::endl;
        return;
    }

    if (channelName.empty() || channelName[0] != '#') {
        std::string error = ":server 403 " + user->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    Channel* channel = getOrCreateChannel(channelName);
    if (!channel) {
        std::cerr << "Error: Failed to create or get channel " << channelName << std::endl;
        return;
    }

    if (channel->hasUser(user)) {
        std::string error = ":server 443 " + user->getNick() + " " + channelName + " :is already on channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    try {
        channel->addUser(user);

        std::string joinMessage = ":" + user->getNick() + "!" + user->getUser() + "@" + user->getHostname() + " JOIN :" + channelName + "\r\n";
        send(fd, joinMessage.c_str(), joinMessage.length(), 0);

        std::string topicMessage = ":server 332 " + user->getNick() + " " + channelName + " :" + channel->getTopic() + "\r\n";
        send(fd, topicMessage.c_str(), topicMessage.length(), 0);

        sendChannelUserList(fd, channel);

        channel->broadcastMessage(joinMessage, user);
    } catch (const std::exception& e) {
        std::cerr << "Error in handleJoin: " << e.what() << std::endl;
        std::string error = ":server 471 " + user->getNick() + " " + channelName + " :Cannot join channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        channel->removeUser(user);
    }
}

Channel* Server::getOrCreateChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        try {
            Channel* newChannel = new Channel(channelName);
            channels[channelName] = newChannel;
            return newChannel;
        } catch (const std::bad_alloc& e) {
            std::cerr << "Error creating new channel: " << e.what() << std::endl;
            return NULL;
        }
    }
    return it->second;
}

void Server::sendChannelUserList(int fd, Channel* channel) {
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser() || !channel) return;

    try {
        std::string userList = ":server 353 " + client->getUser()->getNick() + " = " + channel->getName() + " :";

        std::vector<User*> users = channel->getUsers();
        for (std::vector<User*>::iterator it = users.begin(); it != users.end(); ++it) {
            if (channel->isOperator(*it)) {
                userList += "@";
            }
            userList += (*it)->getNick() + " ";
        }
        userList += "\r\n";

        send(fd, userList.c_str(), userList.length(), 0);

        std::string endOfList = ":server 366 " + client->getUser()->getNick() + " " + channel->getName() + " :End of /NAMES list.\r\n";
        send(fd, endOfList.c_str(), endOfList.length(), 0);
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