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
#include <iomanip>
#include <fstream>
#include "User.hpp"
#include "Client.hpp"
#include "Channel.hpp"

//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color

//-------------------------------------------------------//
class Client; //-> forward declaration
class User; //-> forward declaration
class Channel; //-> forward declaration

class Server //-> class for server
{
	private:
		int Port; //-> server port
		std::string Pass; //-> server password
		int serSocketFd; //-> server socket file descriptor
		static bool Signal; //-> static boolean for signal
		std::vector<Client> clients; //-> vector of clients
		std::vector<struct pollfd> fds; //-> vector of pollfd
		std::map<std::string, Channel*> channels; //-> map of channels

	public:
		Server(); //-> default constructor
		~Server(); //-> destructor

		void serverInit(int port, std::string pass); //-> server initialization
		void serSocket(); //-> server socket creation
		void acceptNewClient(); //-> accept new client
		void receiveNewData(int fd); //-> receive new data from a registered client
		void processClientInput(const char *buff, int id); //-> process the client input
		static void signalHandler(int signum); //-> signal handler
		void printUserParts(User user); //-> print user parts
		void closeFds(); //-> close file descriptors
		void clearClients(int fd); //-> clear clients
		void auth(int fd, std::string pass); //-> authenticate the user
		void sendWelcomeMessages(int fd, const std::string& nickname);
		void handlePing(int fd, const std::string& server);
		void handleWhois(int fd, const std::string& target);
		void handleMode(int fd, const std::string& channel, const std::string& mode);
		void setClientNickname(int fd, const std::string& nick);
		Client* getClientByFd(int fd); //-> get client by file descriptor
		bool isNickInUse(const std::string& nick); //-> check if the nickname is in use
		void printServerState(); //-> print server state in log file
		void setClientUsername(int fd, const std::string& username, const std::string& hostname, const std::string& realname);
		void handleJoin(int fd, const std::string& channelName); //-> handle join command
		Channel* getOrCreateChannel(const std::string& channelName); //-> get or create channel
		void sendChannelUserList(int fd, Channel* channel); //-> send channel user list
		void handleChannelMessage(int senderFd, const std::string& channelName, const std::string& message);
		Channel *getChannel(const std::string& channelName); //-> get channel
		void disconnectClient(int fd); //-> disconnect client
};

#endif