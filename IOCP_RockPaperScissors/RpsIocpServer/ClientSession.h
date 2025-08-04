#pragma once
#include <winsock2.h>
#include <string>
#include "PacketManager.h"

class GameSession;  // 전방 선언

class ClientSession
{
public:
    explicit ClientSession(SOCKET sock);
    ~ClientSession();

    void process();

    void sendLine(const std::string& raw);
    bool readLine(std::string& out);

private:
    SOCKET sock_;
    std::string username_;

    // 추가: 이 세션이 속한 게임 세션 포인터
    GameSession* currentGameSession_ = nullptr;

    // command handlers
    void handleRegister(const PacketManager::Packet&);
    void handleLogin(const PacketManager::Packet&);
    void handleCreate(const PacketManager::Packet&);
    void handleJoin(const PacketManager::Packet&);
    void handleMove(const PacketManager::Packet&);
};