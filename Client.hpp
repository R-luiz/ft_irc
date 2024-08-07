#ifndef CLIENT_HPP
#define CLIENT_HPP

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
#include <memory>
#include "User.hpp"

//-------------------------------------------------------//
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color

//-------------------------------------------------------//

class User;

class Client //-> class for client
{
	private:
		int Fd; //-> client file descriptor
		std::string IPadd; //-> client ip address
		bool authenticated; //-> flag for authentication
		User *user; //-> user object
	public:
		std::string buffer; //-> buffer for client
		Client(); //-> default constructor
		Client(const Client& other);
		Client& operator=(const Client& other); //-> assignment operator
		~Client(); //-> destructor
		int getFd(); //-> getter for fd
		void SetFd(int fd); //-> setter for fd
		void setIpAdd(std::string ipadd); //-> setter for ipadd
		void appendToBuffer(const std::string& data); //-> append data to buffer
		void clearBuffer(); //-> clear buffer
		std::string& getBuffer(); //-> getter for buffer
		bool isAuthenticated(); //-> getter for authenticated
		void setAuthenticated(bool auth); //-> setter for authenticated
		User* getUser(); //-> getter for user
		std::string getIPadd(); //-> getter for ipadd
		void setIPadd(std::string ipadd); //-> setter for ipadd
};

#endif