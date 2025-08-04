#include "GameRoomManager.h"

GameRoomManager& GameRoomManager::instance() 
{
    static GameRoomManager mgr;
    return mgr;
}

GameRoomManager::~GameRoomManager() 
{
    for (auto& [k, v] : rooms_)
        delete v;
}

bool GameRoomManager::createRoom(const std::string& roomId, ClientSession* owner) 
{
    std::lock_guard<std::mutex> lg(mtx_);
    if (rooms_.count(roomId)) return false;
    rooms_[roomId] = new GameSession(owner);
    return true;
}

bool GameRoomManager::joinRoom(const std::string& roomId, ClientSession* challenger) 
{
    std::lock_guard<std::mutex> lg(mtx_);
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) return false;
    it->second->join(challenger);
    // 방은 GameSession::determineResult()에서 제거
    return true;
}

void GameRoomManager::removeRoom(const std::string& roomId) 
{
    std::lock_guard<std::mutex> lg(mtx_);
    delete rooms_[roomId];
    rooms_.erase(roomId);
}

GameSession* GameRoomManager::getSession(const std::string& roomId)
{
    std::lock_guard<std::mutex> lg(mtx_);
    auto it = rooms_.find(roomId);
    return (it == rooms_.end() ? nullptr : it->second);
}
