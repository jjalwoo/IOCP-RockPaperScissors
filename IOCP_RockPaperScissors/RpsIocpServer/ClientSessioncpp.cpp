#include "ClientSession.h"
#include "PacketManager.h"
#include "GameRoomManager.h"
#include "Utils.h"
#include <iostream>

ClientSession::ClientSession(SOCKET sock)
    : sock_(sock) 
{
}

ClientSession::~ClientSession() 
{
    if (sock_ != INVALID_SOCKET)
        closesocket(sock_);
}

void ClientSession::process() 
{
    std::string raw;
    while (readLine(raw)) 
    {
        auto pkt = PacketManager::parse(raw);

        switch (pkt.type) 
        {
        case PacketManager::PacketType::REGISTER:
            handleRegister(pkt);
            break;
        case PacketManager::PacketType::LOGIN:
            handleLogin(pkt);
            break;
        case PacketManager::PacketType::CREATE:
            handleCreate(pkt);
            break;
        case PacketManager::PacketType::JOIN:
            handleJoin(pkt);
            break;
        case PacketManager::PacketType::MOVE:
            handleMove(pkt);
            break;
        default:
            sendLine(PacketManager::serialize( { PacketManager::PacketType::ERROR_PKT, {"Unknown command"} }));
        }
    }
}

void ClientSession::sendLine(const std::string& raw)
{
    std::string data = raw;
    if (data.back() != '\n') data.push_back('\n');
    send(sock_, data.c_str(), (int)data.size(), 0);
}

bool ClientSession::readLine(std::string& out) 
{
    out.clear();
    char ch;
    while (true) {
        int ret = recv(sock_, &ch, 1, 0);
        if (ret <= 0) return false;
        if (ch == '\n') break;
        out += ch;
    }
    return true;
}

// --- 핸들러 stubs ---
void ClientSession::handleRegister(const PacketManager::Packet& pkt) 
{
    // TODO: DB 로직 추가
    sendLine(PacketManager::serialize( { PacketManager::PacketType::REGISTER, {"OK"} }));
}

void ClientSession::handleLogin(const PacketManager::Packet& pkt) 
{
    // TODO: DB 로직 추가
    username_ = pkt.args[0];
    sendLine(PacketManager::serialize( { PacketManager::PacketType::LOGIN, {"OK"} }));
}

void ClientSession::handleCreate(const PacketManager::Packet& pkt) 
{
    const std::string& roomId = pkt.args[0];
    bool ok = GameRoomManager::instance().createRoom(roomId, this);
    if (!ok)
    {
        sendLine(PacketManager::serialize(
            { PacketManager::PacketType::ERROR_PKT, {"Room already exists"} }));
    }
    else
    {
        // 방 생성 성공 → 이 세션이 속한 GameSession* 저장
        currentGameSession_ = GameRoomManager::instance().getSession(roomId);

        sendLine(PacketManager::serialize({ PacketManager::PacketType::WAITING, {} }));
    }

}

void ClientSession::handleJoin(const PacketManager::Packet& pkt)
{
    const std::string& roomId = pkt.args[0];
    bool ok = GameRoomManager::instance().joinRoom(roomId, this);
    if (!ok)
    {
        sendLine(PacketManager::serialize(
            { PacketManager::PacketType::ERROR_PKT, {"No such room"} }));
    }
    else
    {
        // 방 참여 성공 → 이 세션이 속한 GameSession* 저장
        currentGameSession_ =
            GameRoomManager::instance().getSession(roomId);

        // 입장 알림과 START 는 GameSession::join 내부에서 보내줌
    }

}

void ClientSession::handleMove(const PacketManager::Packet& pkt)
{
    if (!currentGameSession_)
    {
        sendLine(PacketManager::serialize(
            { PacketManager::PacketType::ERROR_PKT, {"Not in a room"} }));
        return;
    }

    const std::string& mv = pkt.args[0];
    currentGameSession_->processMove(this, mv);

    // 한 판 끝나고 나면 GameSession 내부에서
    // GameRoomManager::removeRoom(roomId) → 이 방 제거됨.
    // 필요하다면 여기서 currentGameSession_ = nullptr;

}