
#include "Server.hpp"



Server::Server()
{
	serSocketFd = -1;
}

Server::~Server()
{
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
void Server::sendWelcomeMessages(int fd, const std::string& nickname) {
    std::string welcome = "001 " + nickname + " :Welcome to the IRC Network\r\n";
    std::string yourHost = "002 " + nickname + " :Your host is yourservername, running version 1.0\r\n";
    std::string created = "003 " + nickname + " :This server was created sometime\r\n";
    std::string myInfo = "004 " + nickname + " yourservername 1.0 o o\r\n";

    send(fd, welcome.c_str(), welcome.length(), 0);
    send(fd, yourHost.c_str(), yourHost.length(), 0);
    send(fd, created.c_str(), created.length(), 0);
    send(fd, myInfo.c_str(), myInfo.length(), 0);
}

void Server::acceptNewClient()
{
	struct sockaddr_in cliadd;
	struct pollfd NewPoll;
	socklen_t len = sizeof(cliadd);

	int incofd = accept(serSocketFd, (sockaddr *)&(cliadd), &len); //-> accept the new client
	if (incofd == -1)
		{std::cout << "accept() failed" << std::endl; return;}

	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		{std::cout << "fcntl() failed" << std::endl; return;}

	NewPoll.fd = incofd; //-> add the client socket to the pollfd
	NewPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	NewPoll.revents = 0; //-> set the revents to 0

	Client cli; //-> create a new client
	cli.SetFd(incofd); //-> set the client file descriptor
	cli.setIpAdd(inet_ntoa((cliadd.sin_addr))); //-> convert the ip address to string and set it
    cli.getUser()->setFd(incofd);
    clients.push_back(cli); //-> add the client to the vector of clients
	fds.push_back(NewPoll); //-> add the client socket to the pollfd
	std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
	sendWelcomeMessages(incofd, "test");
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
    while (iss >> command) {
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
            iss >> nick;
            setClientNickname(fd, nick);
        }
        else if (command == "USER") {
            // Extract USER information
            std::vector<std::string> userParts;
            std::string part;
            for (int i = 0; i < 4 && iss >> part; ++i) {
                userParts.push_back(part);
            }
            std::string realname;
            std::getline(iss >> std::ws, realname);
            if (!realname.empty() && realname[0] == ':') {
                realname = realname.substr(1); // Remove leading ':'
            }
            if (userParts.size() >= 4) {
                User* user = client->getUser();
                if (user) {
                    user->setUser(userParts[1]);
                    user->setHostname(userParts[2]);
                    user->setRealName(realname);
                    printUserParts(*user);
                    sendWelcomeMessages(fd, user->getNick());
                } else {
                    std::cerr << "Error: User object not found for client." << std::endl;
                }
            }
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

    if (isNickInUse(nick)) {
        std::string error = "433 * " + nick + " :Nickname is already in use\r\n";
        send(fd, error.c_str(), error.length(), 0);
        return;
    }

    std::string response = ":" + nick + " NICK :" + nick + "\r\n";
    client->getUser()->setNick(nick);
    send(fd, response.c_str(), response.length(), 0);

    if (client->isAuthenticated()) {
        sendWelcomeMessages(fd, nick);
    }
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

void Server::receiveNewData(int fd)
{
    char temp_buff[1024];
    ssize_t bytes = recv(fd, temp_buff, sizeof(temp_buff) - 1, 0);
    if (bytes <= 0)
    {
        std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
        close(fd);
    }
    else
    {
        temp_buff[bytes] = '\0';
        Client* client = getClientByFd(fd);
        if (client)
        {
            client->buffer += temp_buff;
            
            size_t pos;
            while ((pos = client->buffer.find("\r\n")) != std::string::npos)
            {
                std::string command = client->buffer.substr(0, pos);
                processClientInput(command.c_str(), fd);
                client->buffer.erase(0, pos + 2);
                if (!getClientByFd(fd))
                {
                    return;
                }
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
	this->Port = port; //-> set the server port
	this->Pass = pass; //-> set the server password
	this->serSocket(); //-> create the server socket

	std::cout << GRE << "Server <" << serSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Waiting to accept a connection...\n";
	while (Server::Signal == false) //-> run the server until the signal is received
	{
		if((poll(&fds[0],fds.size(),-1) == -1) && Server::Signal == false) //-> wait for an event
			throw(std::runtime_error("poll() failed"));

		for (size_t i = 0; i < fds.size(); i++) //-> check all file descriptors
		{
            printServerState();
			if (fds[i].revents & POLLIN)//-> check if there is data to read
			{
				if (fds[i].fd == serSocketFd)
					acceptNewClient(); //-> accept new client
				else
					receiveNewData(fds[i].fd); //-> receive new data from a registered client
			}
		}
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