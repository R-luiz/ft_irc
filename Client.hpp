#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int fd;
    std::string nickname;
    std::string username;
    std::string password;
    bool authenticated;

public:
    Client(int fd);
    ~Client();

    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    bool isAuthenticated() const;

    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setPassword(const std::string& pwd);
    bool authenticate(const std::string& pwd);
    bool isConnected() const;
    void setAuthenticated(bool auth);

    void sendMessage(const std::string& message);
};

#endif