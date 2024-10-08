
#include "Server.hpp"
#include <iostream>

Server::Server()
{
	serSocketFd = -1;
}

Server::~Server()
{
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (*it) {
            delete (*it)->getUser();  // Delete the User object
            delete *it;  // Delete the Client object
        }
    }
    clients.clear();

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) 
    {
        delete it->second;
    }
    channels.clear();
    closeFds();
    fds.clear();
}

void Server::clearClients(int fd) {
    for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); ++it) {
        if (it->fd == fd) 
        {
            fds.erase(it);
            break;
        }
    }
    
    for (std::vector<Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getFd() == fd) 
        {
            clients.erase(it);
            break;
        }
    }
}

void Server::closeFds(){ //-> close the file descriptors
	for(size_t i = 0; i < clients.size(); i++)//-> close the file descriptors of the clients
		close(clients[i]->getFd());
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

    Client* client = new Client();
    client->setFd(incofd);
    client->setIpAdd(inet_ntoa(cliadd.sin_addr));
    client->setUser(new User("Nick_", "Username", incofd));
    client->getUser()->setHostname(inet_ntoa(cliadd.sin_addr));
    client->getUser()->setRealName("Real Name");
    clients.push_back(client);
    fds.push_back(NewPoll);

    std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::printUserParts(User user) 
{
	std::cout << "--> User parts:" << std::endl;
	std::cout << "--> Nick: " << user.getNick() << std::endl;
	std::cout << "--> Username: " << user.getUser() << std::endl;
	std::cout << "--> Hostname: " << user.getHostname() << std::endl;
	std::cout << "--> Fd: " << user.getFd() << std::endl;
	std::cout << "--> End of user parts" << std::endl;
}

void Server::auth(int fd, std::string pass) {
    Client* client = getClientByFd(fd);
    if (!client) 
    {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }
    
    if (pass == this->Pass) 
    {
        client->setAuthenticated(true);
        std::string response = "001 :Password accepted\r\n";
        send(fd, response.c_str(), response.length(), 0);
    } 
    else 
    {
        std::string error = "464 :Password incorrect\r\n";
        send(fd, error.c_str(), error.length(), 0);
        close(fd);
    }
}

void Server::setClientUsername(int fd, const std::string& username, const std::string& hostname, const std::string& realname) {
    Client* client = getClientByFd(fd);
    if (!client) 
    {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }

    User* user = client->getUser();
    if (!user) 
    {
        std::cerr << "Error: User object not found for client." << std::endl;
        return;
    }

    if (!username.empty())
        user->setUser(username);
    if (!hostname.empty())
        user->setHostname(hostname);
    if (!realname.empty())
        user->setRealName(realname);

    if (!user->getNick().empty() && !user->getUser().empty()) 
    {
        if (!client->isAuthenticated()) 
        {
            client->setAuthenticated(true);
            sendWelcomeMessages(fd);
        }
    }

    std::string response = ":server 001 " + user->getNick() + " :Welcome to the IRC Network " + 
                           user->getNick() + "!" + user->getUser() + "@" + user->getHostname() + "\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

bool Server::isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > 9)
        return false;
    for (size_t i = 0; i < nick.length(); ++i) 
    {
        char c = nick[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^' || c == '{' || c == '}'))
            return false;
    }
    return true;
}

void Server::handlePrivmsg(int senderFd, const std::string& target, const std::string& message)
{
    std::cout << "--1---->Handling PRIVMSG from client <" << senderFd << ">" << std::endl;
    Client* sender = getClientByFd(senderFd);
    if (!sender || !sender->getUser()) 
    {
        std::cerr << "Error: Invalid sender in handlePrivmsg" << std::endl;
        return;
    }
    std::string fullMessage = ":" + sender->getUser()->getNick() + "!" +
                              sender->getUser()->getUser() + "@" +
                              sender->getUser()->getHostname() +
                              " PRIVMSG " + target + " :" + message + "\r\n";
    
    std::cout << "Full message to be sent: " << fullMessage << std::endl;
    
    if (target[0] == '#')
    {
        std::cout << "--2------>Channel message" << std::endl;
        Channel* channel = getChannel(target);
        if (channel)
        {
            std::cout << "--3------>Channel found: " << channel->getName() << std::endl;
            std::string senderNick = sender->getUser()->getNick();
            if (channel->hasUser(senderNick))
            {
                std::cout << "--4------>Sender " << senderNick << " is in the channel" << std::endl;
                std::map<std::string, User*> channelUsers = channel->getUsers();
                for (std::map<std::string, User*>::iterator it = channelUsers.begin(); it != channelUsers.end(); ++it)
                {
                    if (it->second && it->second != sender->getUser())
                    {
                        std::cout << "Checking user " << it->first << " before broadcast" << std::endl;
                        Client* recipient = getClientByFd(it->second->getFd());
                        if (recipient && recipient->isAuthenticated())
                        {
                            std::cout << "User " << it->first << " is authenticated and connected" << std::endl;
                        }
                        else
                        {
                            std::cout << "User " << it->first << " is not authenticated or not connected" << std::endl;
                            channel->removeUser(it->first);
                            continue;
                        }
                    }
                }
                try {
                    channel->broadcastMessage(fullMessage, sender->getUser());
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error in handlePrivmsg: " << e.what() << std::endl;
                }
            }
            else
            {
                std::cout << "--5------>Sender " << senderNick << " is not in the channel" << std::endl;
                std::string error = ":server 404 " + senderNick + " " + target + " :Cannot send to channel\r\n";
                send(senderFd, error.c_str(), error.length(), 0);
            }
        } 
        else 
        {
            std::string error = ":server 403 " + sender->getUser()->getNick() + " " + target + " :No such channel\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
        }
    }
    else
    {
        // Private message
        if (target.empty() || !isValidNickname(target)) 
        {
            std::string error = ":server 401 " + sender->getUser()->getNick() + " " + target + " :Invalid nickname\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
            return;
        }
        if (sender->getUser()->getNick() == target) 
        {
            std::string error = ":server 400 " + sender->getUser()->getNick() + " :Cannot send message to yourself\r\n";
            send(senderFd, error.c_str(), error.length(), 0);
            return;
        }

        Client* recipient = NULL;
        for (std::vector<Client *>::iterator it = clients.begin(); it != clients.end(); ++it) 
        {
            if ((*it)->getUser()->getNick() == target) 
            {
                recipient = *it;
                break;
            }
        }

        if (recipient) 
        {
            std::string fullMessage = ":" + sender->getUser()->getNick() + "!" +
                                      sender->getUser()->getUser() + "@" +
                                      sender->getUser()->getHostname() +
                                      " PRIVMSG " + target + " :" + message + "\r\n";
            ssize_t sent = send(recipient->getFd(), fullMessage.c_str(), fullMessage.length(), 0);
            if (sent == -1) 
            {
                std::cerr << "Error sending private message to " << target << ": " << strerror(errno) << std::endl;
                std::string error = ":server 401 " + sender->getUser()->getNick() + " " + target + " :Failed to send message\r\n";
                send(senderFd, error.c_str(), error.length(), 0);
            } 
            else if (static_cast<size_t>(sent) < fullMessage.length())
                std::cerr << "Incomplete private message sent to " << target << std::endl;
            else 
            {
                // Log the private message (optional)
                std::cout << "Private message sent from " << sender->getUser()->getNick() << " to " << target << std::endl;
            }
        } 
        else 
        {
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

    if (!client) 
    {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }
    if (!(iss >> command)) 
    {
        std::cerr << "Empty command received from client <" << fd << ">" << std::endl;
        return;
    }
    if (command == "CAP") 
    {
        std::string capCommand;
        iss >> capCommand;
        if (capCommand == "LS")
            send(fd, "CAP * LS :\r\n", 12, 0);
        else if (capCommand == "END")
            send(fd, "CAP * ACK :multi-prefix\r\n", 25, 0);
    }
    else if (command == "PASS") 
    {
        std::string pass;
        iss >> pass;
        Server::auth(fd, pass);
    }
    else if (!client->isAuthenticated()) 
    {
        std::string error = "451 :You have not registered\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }
    else if (command == "NICK") 
    {
        std::string nick;
        if (iss >> nick)
            setClientNickname(fd, nick);
        else 
        {
            std::string error = "431 :No nickname given\r\n";
            send(fd, error.c_str(), error.length(), 0);
        }
    }
    else if (command == "USER") 
    {
        std::string username, hostname, servername, realname;
        iss >> username >> hostname >> servername;
        std::getline(iss >> std::ws, realname);
        if (!realname.empty() && realname[0] == ':')
            realname = realname.substr(1);
        setClientUsername(fd, username, hostname, realname);
    }
    else if (command == "userhost") 
    {
        std::string username;
        iss >> username;
        setClientUsername(fd, username, "", "");
    }
    else if (command == "WHOIS") 
    {
        std::string target;
        iss >> target;
        handleWhois(fd, target);
    }
    else if (command == "PING") 
    {
        std::string server;
        iss >> server;
        handlePing(fd, server);
    }
    else if (command == "MODE") 
    {
        std::string channelName, modeString;
        std::vector<std::string> args;
        iss >> channelName >> modeString;
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        std::cout << "Received MODE command: " << channelName << " " << modeString;
        for (std::vector<std::string>::const_iterator it = args.begin(); it != args.end(); ++it) {
            std::cout << " " << *it;
        }
        std::cout << std::endl;
        handleMode(fd, channelName, modeString, args);
    }
    else if (command == "JOIN") 
    {
        std::string channelName, key;
        iss >> channelName >> key;
        handleJoin(fd, channelName, key);
    }
    else if (command == "PRIVMSG") 
    {
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
    else if (command == "INVITE")
    {
    std::string nick, channelName;
    iss >> nick >> channelName;
    handleInvite(fd, nick, channelName);
    }
    else if (command == "TOPIC")
    {
    std::string channelName, newTopic;
    iss >> channelName;
    std::getline(iss >> std::ws, newTopic);
    if (!newTopic.empty() && newTopic[0] == ':') {
        newTopic = newTopic.substr(1);
    }
    handleTopic(fd, channelName, newTopic);
    }
    else {
        std::string error = "421 * " + command + " :Unknown command\r\n";
        send(fd, error.c_str(), error.length(), 0);
    }
}

Client* Server::getClientByNick(const std::string& nick) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getUser()->getNick() == nick) {
            return (*it);
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

    if (!channel->hasUser(client->getUser()->getNick())) {
        std::string error = ":server 442 " + client->getUser()->getNick() + " " + channelName + " :You're not on that channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string partMessage = ":" + client->getUser()->getNick() + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() +
                              " PART " + channelName + " :" + reason + "\r\n";
    
    channel->broadcastMessage(partMessage, NULL);
    channel->removeUser(client->getUser()->getNick());

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

    if (!channel->isOperator(kicker->getUser()->getNick())) {
        std::string error = ":server 482 " + kicker->getUser()->getNick() + " " + channelName + " :You're not channel operator\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    Client* target = getClientByNick(targetNick);
    if (!target || !channel->hasUser(targetNick)) {
        std::string error = ":server 441 " + kicker->getUser()->getNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string kickMessage = ":" + kicker->getUser()->getNick() + "!" + kicker->getUser()->getUser() + "@" + kicker->getUser()->getHostname() +
                              " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    channel->broadcastMessage(kickMessage, NULL);

    std::string partMessage = ":" + targetNick + "!" + target->getUser()->getUser() + "@" + target->getUser()->getHostname() +
                              " PART " + channelName + " :Kicked by " + kicker->getUser()->getNick() + "\r\n";
    channel->broadcastMessage(partMessage, NULL);

    channel->removeUser(targetNick);

    send(target->getFd(), kickMessage.c_str(), kickMessage.length(), 0);
    send(target->getFd(), partMessage.c_str(), partMessage.length(), 0);
}

bool Server::isNickInUse(const std::string& nick)
{
    for (std::vector<Client *>::iterator it = clients.begin(); it != clients.end(); ++it) 
    {
        if ((*it)->getUser() && (*it)->getUser()->getNick() == nick)
            return true;
    }
    return false;
}

void Server::setClientNickname(int fd, std::string& nick)
{
    Client* client = getClientByFd(fd);
    if (!client) 
    {
        std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        return;
    }

    if (nick.empty()) 
    {
        std::string error = "431 :No nickname given\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }
    
    if (isNickInUse(nick))
    {
        std::string error = "433 * " + nick + " :Nickname is already in use\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string oldNick = client->getUser()->getNick();
    client->getUser()->setNick(nick);
    client->setNickSet(true);

    std::string response;
    if (oldNick.empty())
        response = ":" + nick + " NICK :" + nick + "\r\n";
    else
        response = ":" + oldNick + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() + " NICK :" + nick + "\r\n";
    send(fd, response.c_str(), response.length(), 0);

    // Update nickname in all channels the user is in
    std::map<std::string, Channel*>::iterator it;
    for (it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* channel = it->second;
        if (channel->hasUser(oldNick))
        {
            channel->removeUser(oldNick);
            channel->addUser(client->getUser(), channel->isOperator(oldNick));
        }
    }

    checkRegistration(client);
}

void Server::sendWelcomeMessages(int fd)
{
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

void Server::handleMode(int fd, const std::string& channel, const std::string& mode) 
{
    // Implement channel mode handling
    std::string response = ":server 324 " + channel + " " + mode + "\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

void Server::handleWhois(int fd, const std::string& target) 
{
    // Implement WHOIS command
    std::string response = ":server 311 " + target + " " + target + " username hostname * :realname\r\n";
    response += ":server 318 " + target + " :End of /WHOIS list.\r\n";
    send(fd, response.c_str(), response.length(), 0);
}

void Server::handlePing(int fd, const std::string& server) 
{
    std::string response = "PONG :" + server + "\r\n";
    ssize_t sent = send(fd, response.c_str(), response.length(), 0);
    if (sent == -1) 
    {
        // Handle send error
        std::cerr << "Error sending PONG response" << std::endl;
    } 
    else if (static_cast<size_t>(sent) < response.length()) 
    {
        // Handle partial send
        std::cerr << "Partial PONG response sent" << std::endl;
    }
}

Client* Server::getClientByFd(int fd) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) 
    {
        if ((*it)->getFd() == fd) 
        {
            return (*it);
        }
    }
    return NULL;
}

void Server::disconnectClient(int fd) {
    std::vector<struct pollfd>::iterator it;
    for (it = fds.begin(); it != fds.end(); ++it) 
    {
        if (it->fd == fd) 
        {
            fds.erase(it);
            break;
        }
    }

    std::vector<Client*>::iterator client_it;
    for (client_it = clients.begin(); client_it != clients.end(); ++client_it) 
    {
        if ((*client_it)->getFd() == fd) 
        {
            // Remove user from channels
            std::map<std::string, Channel*>::iterator channel_it;
            for (channel_it = channels.begin(); channel_it != channels.end(); ++channel_it)
                channel_it->second->removeUser((*client_it)->getUser()->getNick());
            
            delete *client_it;  // This will also delete the User object
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
            while ((pos = client->getBuffer().find("\r\n")) != std::string::npos)
            {
                std::string command = client->getBuffer().substr(0, pos);
                processClientInput(command.c_str(), fd);
                client->getBuffer().erase(0, pos + 2);
            }
        }
        else
        {
            std::cerr << "Error: Client with fd " << fd << " not found." << std::endl;
        }
    }
}
void Server::printServerState() 
{
    static int updateCount = 0;
    updateCount++;

    std::ofstream outFile("server_state.log", std::ios::app);
    if (!outFile.is_open()) 
    {
        std::cerr << "Failed to open server_state.log for writing." << std::endl;
        return;
    }

    outFile << "==== Server State (Update #" << updateCount << ") ====" << std::endl;
    outFile << "Server Socket FD: " << serSocketFd << std::endl;
    outFile << "Port: " << Port << std::endl;
    outFile << "Number of clients: " << clients.size() << std::endl;
    outFile << std::endl;
    outFile << "=== Clients ===" << std::endl;
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) 
    {
        Client& client = *(*it);
        outFile << "Client FD: " << client.getFd() << std::endl;
        outFile << "IP Address: " << client.getIPadd() << std::endl;
        outFile << "Authenticated: " << (client.isAuthenticated() ? "Yes" : "No") << std::endl;
        
        User* user = client.getUser();
        if (user) 
        {
            outFile << "  Nickname: " << user->getNick() << std::endl;
            outFile << "  Username: " << user->getUser() << std::endl;
            outFile << "  Hostname: " << user->getHostname() << std::endl;
            outFile << "  Realname: " << user->getRealName() << std::endl;
        } 
        else
            outFile << "  User data not available" << std::endl;
        outFile << "  Buffer: " << (client.getBuffer().empty() ? "Empty" : client.getBuffer()) << std::endl;
        outFile << std::endl;
    }
    outFile << "=== Active File Descriptors ===" << std::endl;
    for (std::vector<struct pollfd>::const_iterator it = fds.begin(); it != fds.end(); ++it)
        outFile << "FD: " << it->fd << ", Events: " << it->events << ", Revents: " << it->revents << std::endl;
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
                    --i;
                }
            }
        }
    }
}

Channel *Server::getChannel(const std::string& channelName) 
{
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) 
    {
        std::cerr << "Error: Channel " << channelName << " not found." << std::endl;
        return NULL;
    }
    return it->second;
}

void Server::handleChannelMessage(int senderFd, const std::string& channelName, const std::string& message) 
{
    Client* client = getClientByFd(senderFd);
    if (!client || !client->getUser()) return;

    Channel* channel = getChannel(channelName);
    if (!channel) return;

    User* sender = client->getUser();
    std::string formattedMessage = ":" + sender->getNick() + "!" + sender->getUser() + "@" + sender->getHostname() + " PRIVMSG " + channelName + " :" + message + "\r\n";

    channel->broadcastMessage(formattedMessage, sender);
}

void Server::handleJoin(int fd, const std::string& channelName, const std::string& key) 
{
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser()) 
    {
        std::cerr << "Invalid client in handleJoin" << std::endl;
        return;
    }

    Channel* channel = getOrCreateChannel(channelName);
    if (!channel) 
    {
        std::cerr << "Failed to create or get channel " << channelName << std::endl;
        return;
    }
    User* user = client->getUser();
    std::string nick = user->getNick();
    
    if (channel->hasUser(nick))
    {
        std::string error = ":server 443 " + nick + " " + channelName + " :is already on channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }
    
    if (channel->isInviteOnly() && !channel->isInvited(nick))
    {
        std::string error = ":server 473 " + nick + " " + channelName + " :Cannot join channel (+i)\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->checkKey(key))
    {
        std::string error = ":server 475 " + nick + " " + channelName + " :Cannot join channel (+k)\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel->isAtCapacity())
    {
        std::string error = ":server 471 " + nick + " " + channelName + " :Cannot join channel (+l)\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }
    
    bool isFirstUser = channel->getUsers().empty();
    channel->addUser(user, isFirstUser);

    std::string joinMessage = ":" + nick + "!" + user->getUser() + "@" + user->getHostname() + " JOIN :" + channelName + "\r\n";
    send(fd, joinMessage.c_str(), joinMessage.length(), 0);  // Send the message to the user who is joining
    channel->broadcastMessage(joinMessage, user);  // Broadcast to all other users

    std::string topicMessage = ":server 332 " + nick + " " + channelName + " :" + channel->getTopic() + "\r\n";
    send(fd, topicMessage.c_str(), topicMessage.length(), 0);

    sendChannelUserList(fd, channel);
}

Channel* Server::getOrCreateChannel(const std::string& channelName) 
{
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        Channel* newChannel = new Channel(channelName);
        channels[channelName] = newChannel;
        return newChannel;
    }
    return it->second;
}

void Server::sendChannelUserList(int fd, Channel* channel) 
{
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser() || !channel) 
    {
        std::cerr << "Invalid client or channel in sendChannelUserList" << std::endl;
        return;
    }

    try 
    {
        std::string userList = ":server 353 " + client->getUser()->getNick() + " = " + channel->getName() + " :";
        std::map<std::string, User*> users = channel->getUsers();
        
        for (std::map<std::string, User*>::const_iterator it = users.begin(); it != users.end(); ++it) 
        {
            if (it->second) 
            {
                if (channel->isOperator(it->first)) 
                {
                    userList += "@";
                }
                userList += it->first + " ";
            }
        }
        userList += "\r\n";

        if (send(fd, userList.c_str(), userList.length(), 0) == -1) 
        {
            std::cerr << "Error sending user list to client" << std::endl;
        }

        std::string endOfList = ":server 366 " + client->getUser()->getNick() + " " + channel->getName() + " :End of /NAMES list.\r\n";
        if (send(fd, endOfList.c_str(), endOfList.length(), 0) == -1) 
        {
            std::cerr << "Error sending end of list to client" << std::endl;
        }
    } 
    catch (const std::exception& e) 
    {
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
	try
    {
		signal(SIGINT, Server::signalHandler); //-> catch the signal (ctrl + c)
		signal(SIGQUIT, Server::signalHandler); //-> catch the signal (ctrl + \)
		ser.serverInit(port, pass); //-> initialize the server
	}
	catch(const std::exception& e)
    {
		ser.closeFds(); //-> close the file descriptors
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}

void Server::handleInvite(int fd, const std::string& nick, const std::string& channelName) {
    Client* inviter = getClientByFd(fd);
    if (!inviter || !inviter->getUser()) 
    {
        std::cerr << "Invalid inviter in handleInvite" << std::endl;
        return;
    }

    Channel* channel = getChannel(channelName);
    if (!channel)
    {
        std::string error = ":server 403 " + inviter->getUser()->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(inviter->getUser()->getNick()))
    {
        std::string error = ":server 482 " + inviter->getUser()->getNick() + " " + channelName + " :You're not channel operator\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    Client* invitee = getClientByNick(nick);
    if (!invitee)
    {
        std::string error = ":server 401 " + inviter->getUser()->getNick() + " " + nick + " :No such nick\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (channel->hasUser(nick))
    {
        std::string error = ":server 443 " + inviter->getUser()->getNick() + " " + nick + " " + channelName + " :is already on channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string inviteMsg = ":" + inviter->getUser()->getNick() + "!" + inviter->getUser()->getUser() + "@" + inviter->getUser()->getHostname() +
                            " INVITE " + nick + " :" + channelName + "\r\n";
    send(invitee->getFd(), inviteMsg.c_str(), inviteMsg.length(), 0);

    std::string confirmMsg = ":server 341 " + inviter->getUser()->getNick() + " " + nick + " " + channelName + "\r\n";
    send(fd, confirmMsg.c_str(), confirmMsg.length(), 0);

    channel->addInvitedUser(nick);
}

void Server::handleTopic(int fd, const std::string& channelName, const std::string& newTopic) {
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser())
    {
        std::cerr << "Invalid client in handleTopic" << std::endl;
        return;
    }

    Channel* channel = getChannel(channelName);
    if (!channel)
    {
        std::string error = ":server 403 " + client->getUser()->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->hasUser(client->getUser()->getNick()))
    {
        std::string error = ":server 442 " + client->getUser()->getNick() + " " + channelName + " :You're not on that channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (newTopic.empty())
    {
        // User is requesting the current topic
        std::string currentTopic = channel->getTopic();
        std::string response = ":server 332 " + client->getUser()->getNick() + " " + channelName + " :" + currentTopic + "\r\n";
        send(fd, response.c_str(), response.length(), 0);
    }
    else
    {
        // User is trying to set a new topic
        if (channel->isTopicRestricted() && !channel->isOperator(client->getUser()->getNick())) {
            std::string error = ":server 482 " + client->getUser()->getNick() + " " + channelName + " :You're not channel operator\r\n";
            send(fd, error.c_str(), error.length(), 0);
            return;
        }

        channel->setTopic(newTopic);
        std::string topicMessage = ":" + client->getUser()->getNick() + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() +
                                   " TOPIC " + channelName + " :" + newTopic + "\r\n";
        channel->broadcastMessage(topicMessage, NULL);
    }
}

void Server::handleMode(int fd, const std::string& channelName, const std::string& modeString, const std::vector<std::string>& args) {
    std::cout << "Handling MODE command: " << channelName << " " << modeString << std::endl;
    
    Client* client = getClientByFd(fd);
    if (!client || !client->getUser()) {
        std::cerr << "Invalid client in handleMode" << std::endl;
        return;
    }

    Channel* channel = getChannel(channelName);
    if (!channel) {
        std::string error = ":server 403 " + client->getUser()->getNick() + " " + channelName + " :No such channel\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    if (!channel->isOperator(client->getUser()->getNick())) {
        std::string error = ":server 482 " + client->getUser()->getNick() + " " + channelName + " :You're not channel operator\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    bool adding = true;
    size_t argIndex = 0;
    std::string modeChanges;
    std::string modeArgs;

    for (size_t i = 0; i < modeString.length(); ++i) {
        char mode = modeString[i];
        if (mode == '+') {
            adding = true;
            modeChanges += '+';
        } else if (mode == '-') {
            adding = false;
            modeChanges += '-';
        } else {
            modeChanges += mode;
            switch (mode) {
                case 'i': // Invite-only
                    channel->setInviteOnly(adding);
                    std::cout << "Set invite-only mode to " << (adding ? "on" : "off") << " for channel " << channelName << std::endl;
                    break;
                case 't': // Topic restriction
                    channel->setTopicRestricted(adding);
                    std::cout << "Set topic restriction mode to " << (adding ? "on" : "off") << " for channel " << channelName << std::endl;
                    break;
                case 'k': // Channel key (password)
                    if (adding && argIndex < args.size()) {
                        channel->setKey(args[argIndex]);
                        modeArgs += " " + args[argIndex];
                        ++argIndex;
                        std::cout << "Set channel key for " << channelName << std::endl;
                    } else if (!adding) {
                        channel->setKey("");
                        std::cout << "Removed channel key for " << channelName << std::endl;
                    }
                    break;
                case 'o': // Operator status
                    if (argIndex < args.size()) {
                        channel->setOperator(args[argIndex], adding);
                        modeArgs += " " + args[argIndex];
                        ++argIndex;
                        std::cout << (adding ? "Set" : "Removed") << " operator status for " << args[argIndex-1] << " in channel " << channelName << std::endl;
                    }
                    break;
                case 'l': // User limit
                    if (adding && argIndex < args.size()) {
                        channel->setUserLimit(std::atoi(args[argIndex].c_str()));
                        modeArgs += " " + args[argIndex];
                        ++argIndex;
                        std::cout << "Set user limit to " << args[argIndex-1] << " for channel " << channelName << std::endl;
                    } else if (!adding) {
                        channel->setUserLimit(0);
                        std::cout << "Removed user limit for channel " << channelName << std::endl;
                    }
                    break;
                default:
                    std::cout << "Unknown mode: " << mode << std::endl;
                    break;
            }
        }
    }

    // Create the mode change message
    std::string modeChangeMsg = ":" + client->getUser()->getNick() + "!" + client->getUser()->getUser() + "@" + client->getUser()->getHostname() +
                                " MODE " + channelName + " " + modeChanges + modeArgs + "\r\n";

    // Broadcast mode changes to all users in the channel, including the sender
    channel->broadcastMessage(modeChangeMsg, NULL);

    // We no longer need to send a separate message to the client who set the mode
    // as they will receive the broadcast message
}