#pragma once
#include <winsock2.h>
#include <string>

class Server 
{
public:
    Server(const std::string& ip, int port);
    ~Server();

    bool start();    // WSAStartup, socket(), bind(), listen()
    void run();      // accept loop

private:
    std::string ip_;
    int port_;
    SOCKET listenSock_;
};