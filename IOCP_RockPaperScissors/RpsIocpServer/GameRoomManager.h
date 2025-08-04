#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include "GameSession.h"

class ClientSession;
class GameSession;

class GameRoomManager 
{
public:
    static GameRoomManager& instance();

    bool createRoom(const std::string& roomId, ClientSession* owner);
    bool joinRoom(const std::string& roomId, ClientSession* challenger);
    void removeRoom(const std::string& roomId);

    std::unordered_map<std::string, GameSession*> rooms_;
    GameSession* getSession(const std::string& roomId);

private:
    GameRoomManager() = default;
    ~GameRoomManager();

    std::mutex mtx_;
    
};