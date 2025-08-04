#pragma once
#include <vector>
#include <unordered_map>
#include <mutex>
#include "ClientSession.h"

class GameSession 
{
public:
    explicit GameSession(ClientSession* p1);
    ~GameSession();

    void join(ClientSession* p2);
    void processMove(ClientSession* player, const std::string& move);

private:
    std::vector<ClientSession*> players_;
    std::unordered_map<ClientSession*, std::string> moves_;
    std::mutex mtx_;

    void start();
    void determineResult();
};

