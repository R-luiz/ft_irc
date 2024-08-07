#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector> //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h> //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h> //-> for fcntl()
#include <unistd.h> //-> for close()
#include <arpa/inet.h> //-> for inet_ntoa()
#include <poll.h> //-> for poll()
#include <csignal> //-> for signal()
#include <cstring> //-> for memset()
#include <string> //-> for string
#include <cstdlib> //-> for atoi() 
#include <sstream> //-> for stringstream
#include "User.hpp"
#include "Client.hpp"

//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color

//-------------------------------------------------------//
class Client; //-> forward declaration
class User; //-> forward declaration

class Server //-> class for server
{
	private:
		int Port; //-> server port
		std::string Pass; //-> server password
		int SerSocketFd; //-> server socket file descriptor
		static bool Signal; //-> static boolean for signal
		std::vector<Client> clients; //-> vector of clients
		std::vector<struct pollfd> fds; //-> vector of pollfd
		std::vector<User> users; //-> vector of users
	public:
		Server(); //-> default constructor

		void ServerInit(int port, std::string pass); //-> server initialization
		void SerSocket(); //-> server socket creation
		void AcceptNewClient(); //-> accept new client
		void ReceiveNewData(int fd); //-> receive new data from a registered client
		void ProcessClientInput(const char *buff, int id); //-> process the client input
		static void SignalHandler(int signum); //-> signal handler
		void PrintUserParts(User user); //-> print user parts
		void CloseFds(); //-> close file descriptors
		void ClearClients(int fd); //-> clear clients
};

#endif