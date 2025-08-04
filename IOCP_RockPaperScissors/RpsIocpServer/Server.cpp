#include "Server.h"
#include "ClientSession.h"
#include <iostream>
#include <thread>
#pragma comment(lib, "ws2_32.lib")  // Winsock 라이브러리 연결

#include <ws2tcpip.h>   

Server::Server(const std::string& ip, int port)
    : ip_(ip), port_(port), listenSock_(INVALID_SOCKET) 
{
}

Server::~Server()
{
    if (listenSock_ != INVALID_SOCKET) 
    {
        closesocket(listenSock_);
    }
    WSACleanup();
}

bool Server::start() 
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock_ == INVALID_SOCKET) 
    {
        std::cerr << "socket() failed\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    int opt = 1;
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR,
        (char*)&opt, sizeof(opt));

    if (bind(listenSock_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
    {
        std::cerr << "bind() failed\n";
        return false;
    }

    if (listen(listenSock_, SOMAXCONN) == SOCKET_ERROR) 
    {
        std::cerr << "listen() failed\n";
        return false;
    }

    std::cout << "Server listening on " << ip_ << ":" << port_ << "\n";
    return true;
}

void Server::run() 
{
    while (true) 
    {
        SOCKET clientSock = accept(listenSock_, nullptr, nullptr);

        if (clientSock == INVALID_SOCKET) 
            continue;

        std::thread([clientSock]() 
            {
            ClientSession session(clientSock);
            session.process();
            }).detach();
    }
}