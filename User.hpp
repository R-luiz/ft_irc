#ifndef USER_HPP
#define USER_HPP

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
#include "Server.hpp"

//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color
//-------------------------------------------------------//

class Server;

class User //-> class for user
{
	private:
		std::string nickname;
		std::string username;
		std::string password;
		std::string hostname;
		std::string realname;
		int fd;
	public:
		User(); //-> default constructor
		User(std::string nick, std::string user, std::string pass); //-> constructor with parameters
		~User(); //-> destructor
		std::string getNick(); //-> getter for nickname
		std::string getUser(); //-> getter for username
		std::string getPass(); //-> getter for password
		std::string getHostname(); //-> getter for hostname
		std::string getRealName(); //-> getter for realname
		int getFd(); //-> getter for fd
		void setNick(std::string nick); //-> setter for nickname
		void setUser(std::string user); //-> setter for username
		void setPass(std::string pass); //-> setter for password
		void setFd(int fd); //-> setter for fd
		void setHostname(std::string host); //-> setter for hostname
		void setRealName(std::string real); //-> setter for realname
};

#endif
