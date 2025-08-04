#include "GameSession.h"
#include "GameRoomManager.h"
#include "PacketManager.h"
#include <algorithm>

GameSession::GameSession(ClientSession* p1)
{
    players_.push_back(p1);
}

GameSession::~GameSession() = default;

void GameSession::join(ClientSession* p2) 
{
    {
        std::lock_guard<std::mutex> lg(mtx_);
        players_.push_back(p2);
    }
    // 둘 다 입장 알림
    for (auto* p : players_)
    {
        p->sendLine(PacketManager::serialize(
            { PacketManager::PacketType::OPPONENT_JOINED, {} }));
    }
    start();
}

void GameSession::start() 
{
    for (auto* p : players_) 
    {
        p->sendLine(PacketManager::serialize(
            { PacketManager::PacketType::START, {} }));
    }
}

void GameSession::processMove(ClientSession* player, const std::string& move) 
{
    std::lock_guard<std::mutex> lg(mtx_);
    moves_[player] = move;
    if (moves_.size() < 2) return;
    determineResult();
}

void GameSession::determineResult() 
{
    // 플레이어 순서 보장되지 않으니 벡터로 찾기
    auto p1 = players_[0], p2 = players_[1];
    auto m1 = moves_[p1], m2 = moves_[p2];

    // 결과 판단
    int outcome = 0; // 0 draw, 1 p1 win, -1 p2 win
    if (m1 == m2)        outcome = 0;
    else if ((m1 == "R" && m2 == "S") ||
        (m1 == "S" && m2 == "P") ||
        (m1 == "P" && m2 == "R"))
        outcome = 1;
    else outcome = -1;

    // p1에게
    std::string r1 = (outcome == 1 ? "WIN"
        : outcome == 0 ? "DRAW"
        : "LOSE");
    // p2에게
    std::string r2 = (outcome == 1 ? "LOSE"
        : outcome == 0 ? "DRAW"
        : "WIN");

    p1->sendLine(PacketManager::serialize(
        { PacketManager::PacketType::RESULT, {r1} }));
    p2->sendLine(PacketManager::serialize(
        { PacketManager::PacketType::RESULT, {r2} }));

    // 방 제거
    GameRoomManager::instance()
        .removeRoom( /* roomId? */
            ""); // 실제로는 roomId를 저장해두고 사용
}