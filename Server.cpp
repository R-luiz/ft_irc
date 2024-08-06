
#include "Server.hpp"

Client::Client() {}

void Client::setIpAdd(std::string ipadd)
{
	IPadd = ipadd;
}

void Client::SetFd(int fd)
{
	Fd = fd;
}

int Client::getFd()
{
	return Fd;
}

Server::Server()
{
	SerSocketFd = -1;
}

void Server::ClearClients(int fd){ //-> clear the clients
	for(size_t i = 0; i < fds.size(); i++)//-> remove the client from the pollfd
	{ 
		if (fds[i].fd == fd)
		{
			fds.erase(fds.begin() + i); 
			break;
		}
	}
	for(size_t i = 0; i < clients.size(); i++)
	{ //-> remove the client from the vector of clients
		if (clients[i].getFd() == fd)
		{
			clients.erase(clients.begin() + i); 
			break;
		}
	}

}

void Server::CloseFds(){ //-> close the file descriptors
	for(size_t i = 0; i < clients.size(); i++)//-> close the file descriptors of the clients
		close(clients[i].getFd());
	if (SerSocketFd != -1) //-> close the server socket file descriptor
		close(SerSocketFd);
}

bool Server::Signal = false; //-> initialize the static boolean
void Server::SignalHandler(int signum) //-> signal handler
{ 
	std::cout << RED << "Signal " << signum << " received" << WHI << std::endl;
	Signal = true;
}

void Server::SerSocket()
{
	struct sockaddr_in add;
	struct pollfd NewPoll;
	add.sin_family = AF_INET; //-> set the address family to ipv4
	add.sin_port = htons(this->Port); //-> convert the port to network byte order (big endian)
	add.sin_addr.s_addr = INADDR_ANY; //-> set the address to any local machine address

	SerSocketFd = socket(AF_INET, SOCK_STREAM, 0); //-> create the server socket
	if(SerSocketFd == -1) //-> check if the socket is created
		throw(std::runtime_error("faild to create socket"));

	int en = 1;
	if(setsockopt(SerSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) //-> set the socket option (SO_REUSEADDR) to reuse the address
		throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));
	if (fcntl(SerSocketFd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));
	if (bind(SerSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) //-> bind the socket to the address
		throw(std::runtime_error("faild to bind socket"));
	if (listen(SerSocketFd, SOMAXCONN) == -1) //-> listen for incoming connections and making the socket a passive socket
		throw(std::runtime_error("listen() failed"));

	NewPoll.fd = SerSocketFd; //-> add the server socket to the pollfd
	NewPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	NewPoll.revents = 0; //-> set the revents to 0
	fds.push_back(NewPoll); //-> add the server socket to the pollfd
}
void Server::AcceptNewClient()
{
	Client cli; //-> create a new client
	struct sockaddr_in cliadd;
	struct pollfd NewPoll;
	socklen_t len = sizeof(cliadd);

	int incofd = accept(SerSocketFd, (sockaddr *)&(cliadd), &len); //-> accept the new client
	if (incofd == -1)
		{std::cout << "accept() failed" << std::endl; return;}

	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) //-> set the socket option (O_NONBLOCK) for non-blocking socket
		{std::cout << "fcntl() failed" << std::endl; return;}

	NewPoll.fd = incofd; //-> add the client socket to the pollfd
	NewPoll.events = POLLIN; //-> set the event to POLLIN for reading data
	NewPoll.revents = 0; //-> set the revents to 0

	cli.SetFd(incofd); //-> set the client file descriptor
	cli.setIpAdd(inet_ntoa((cliadd.sin_addr))); //-> convert the ip address to string and set it
	clients.push_back(cli); //-> add the client to the vector of clients
	fds.push_back(NewPoll); //-> add the client socket to the pollfd

	std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::PrintUserParts(User user) {
	std::cout << "--> User parts:" << std::endl;
	std::cout << "--> Nick: " << user.getNick() << std::endl;
	std::cout << "--> Username: " << user.getUser() << std::endl;
	std::cout << "--> Hostname: " << user.getHostname() << std::endl;
	std::cout << "--> Fd: " << user.getFd() << std::endl;
	std::cout << "--> End of user parts" << std::endl;
}

User::User() {}

User::User(std::string nick, std::string user, std::string pass) {
	nickname = nick;
	username = user;
	password = pass;
}

User::~User() {}

std::string User::getNick() {
	return nickname;
}

std::string User::getUser() {
	return username;
}

std::string User::getPass() {
	return password;
}

std::string User::getHostname() {
	return hostname;
}

int User::getFd() {
	return fd;
}

void User::setNick(std::string nick) {
	nickname = nick;
}

void User::setUser(std::string user) {
	username = user;
}

void User::setPass(std::string pass) {
	password = pass;
}

void User::setHostname(std::string host) {
	hostname = host;
}

void User::setFd(int fd) {
	this->fd = fd;
}

void Server::ProcessClientInput(const char *buff, int fd) 
{
	std::string input(buff);
        std::istringstream iss(input);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.find("CAP LS") != std::string::npos) {
                // Handle capability negotiation
                send(fd, "CAP * LS :\r\n", 12, 0);
            }
            else if (line.find("NICK") == 0) {
                // Extract nickname
                std::string nick = line.substr(5); // Assumes "NICK " is always 5 characters
                // Handle nickname setting
                // setClientNickname(fd, nick);
            }
            else if (line.find("USER") == 0) {
                // Extract USER information
                std::istringstream userStream(line);
                std::vector<std::string> userParts;
                std::string part;
                while (userStream >> part) {
                    userParts.push_back(part);
                }
				User user = User();
				user.setNick(userParts[1]);
				user.setUser(userParts[2]);
				user.setHostname(userParts[3]);
				user.setFd(fd);
				user.setPass("password123");
				users.push_back(user);
				// Print user parts
				PrintUserParts(user);
            }
        }
}

void Server::ReceiveNewData(int fd)
{
	char buff[1024]; //-> create a buffer to store the received data
	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1 , 0); //-> receive the data
	if(bytes <= 0){ //-> check if the client disconnected
		std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
		ClearClients(fd); //-> clear the client
		close(fd); //-> close the client socket
	}
	else{ //-> print the received data
		buff[bytes] = '\0';
		std::cout << YEL << "Client <" << fd << "> Data: " << WHI << buff;
		ProcessClientInput(buff, fd);
		//here you can add your code to process the received data: parse, check, authenticate, handle the command, etc...
	}
}

void Server::ServerInit(int port, std::string pass)
{
	this->Port = port; //-> set the server port
	this->Pass = pass; //-> set the server password
	this->SerSocket(); //-> create the server socket

	std::cout << GRE << "Server <" << SerSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Waiting to accept a connection...\n";
	while (Server::Signal == false) //-> run the server until the signal is received
	{
		if((poll(&fds[0],fds.size(),-1) == -1) && Server::Signal == false) //-> wait for an event
			throw(std::runtime_error("poll() faild"));

		for (size_t i = 0; i < fds.size(); i++) //-> check all file descriptors
		{
			if (fds[i].revents & POLLIN)//-> check if there is data to read
			{
				if (fds[i].fd == SerSocketFd)
					AcceptNewClient(); //-> accept new client
				else
					ReceiveNewData(fds[i].fd); //-> receive new data from a registered client
			}
		}
	}
	CloseFds(); //-> close the file descriptors when the server stops
}

bool isPortValid(std::string port){
	return (port.find_first_not_of("0123456789") == std::string::npos && \
	std::atoi(port.c_str()) >= 1024 && std::atoi(port.c_str()) <= 65535);
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
		signal(SIGINT, Server::SignalHandler); //-> catch the signal (ctrl + c)
		signal(SIGQUIT, Server::SignalHandler); //-> catch the signal (ctrl + \)
		ser.ServerInit(port, pass); //-> initialize the server
	}
	catch(const std::exception& e){
		ser.CloseFds(); //-> close the file descriptors
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}