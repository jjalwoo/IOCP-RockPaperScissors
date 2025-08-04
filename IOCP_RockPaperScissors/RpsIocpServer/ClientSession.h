#pragma once
#include <winsock2.h>
#include <string>
#include "PacketManager.h"

class GameSession;  // ���� ����

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

    // �߰�: �� ������ ���� ���� ���� ������
    GameSession* currentGameSession_ = nullptr;

    // command handlers
    void handleRegister(const PacketManager::Packet&);
    void handleLogin(const PacketManager::Packet&);
    void handleCreate(const PacketManager::Packet&);
    void handleJoin(const PacketManager::Packet&);
    void handleMove(const PacketManager::Packet&);
};