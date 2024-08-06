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

//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color
//-------------------------------------------------------//
class User //-> class for user
{
	private:
		std::string nickname;
		std::string username;
		std::string password;
		std::string hostname;
		int fd;
	public:
		User(); //-> default constructor
		User(std::string nick, std::string user, std::string pass); //-> constructor with parameters
		~User(); //-> destructor
		std::string getNick(); //-> getter for nickname
		std::string getUser(); //-> getter for username
		std::string getPass(); //-> getter for password
		std::string getHostname(); //-> getter for hostname
		int getFd(); //-> getter for fd
		void setNick(std::string nick); //-> setter for nickname
		void setUser(std::string user); //-> setter for username
		void setPass(std::string pass); //-> setter for password
		void setFd(int fd); //-> setter for fd
		void setHostname(std::string host); //-> setter for hostname
};

//-------------------------------------------------------//
class Client //-> class for client
{
	private:
		int Fd; //-> client file descriptor
		std::string IPadd; //-> client ip address
	public:
		Client(); //-> default constructor
		int getFd(); //-> getter for fd

		void SetFd(int fd); //-> setter for fd
		void setIpAdd(std::string ipadd); //-> setter for ipadd
};

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
