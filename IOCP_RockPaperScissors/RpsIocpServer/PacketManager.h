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
        WAITING     // ��� ȭ������ �̵�
    };

    struct Packet 
    {
        PacketType type;
        std::vector<std::string> args;
    };

    // ���� ���ڿ� �� Packet
    Packet parse(const std::string& raw);

    // Packet �� ���ۿ� ���ڿ� (���� '\n' ����)
    std::string serialize(const Packet& pkt);

}