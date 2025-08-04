#pragma once
#include <string>
#include <vector>

namespace PacketManager 
{

    enum class PacketType 
    {
        REGISTER,
        LOGIN,
        CREATE, 
        JOIN,
        MOVE, 
        RESULT,
        OPPONENT_JOINED, 
        START, 
        ERROR_PKT, 
        UNKNOWN, 
        WAITING     // 대기 화면으로 이동
    };

    struct Packet 
    {
        PacketType type;
        std::vector<std::string> args;
    };

    // 원시 문자열 → Packet
    Packet parse(const std::string& raw);

    // Packet → 전송용 문자열 (끝에 '\n' 포함)
    std::string serialize(const Packet& pkt);

}