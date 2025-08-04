// src/PacketManager.cpp
#include "PacketManager.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace PacketManager
{

    // 대소문자 무시 비교
    static bool iequals(const std::string& a, const std::string& b) 
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) 
        {
            if (std::tolower(a[i]) != std::tolower(b[i]))
            {
                return false;
            }
        }
        return true;
    }

    Packet parse(const std::string& raw) 
    {
        std::istringstream iss(raw);
        std::string cmd;
        iss >> cmd;

        Packet pkt;
        if (iequals(cmd, "REGISTER"))           pkt.type = PacketType::REGISTER;
        else if (iequals(cmd, "LOGIN"))         pkt.type = PacketType::LOGIN;
        else if (iequals(cmd, "CREATE"))        pkt.type = PacketType::CREATE;
        else if (iequals(cmd, "JOIN"))          pkt.type = PacketType::JOIN;
        else if (iequals(cmd, "MOVE"))          pkt.type = PacketType::MOVE;
        else if (iequals(cmd, "RESULT"))        pkt.type = PacketType::RESULT;
        else if (iequals(cmd, "OPPONENT_JOINED")) pkt.type = PacketType::OPPONENT_JOINED;
        else if (iequals(cmd, "START"))         pkt.type = PacketType::START;
        else if (iequals(cmd, "ERROR"))         pkt.type = PacketType::ERROR_PKT;
        else                                     pkt.type = PacketType::UNKNOWN;

        std::string token;
        while (iss >> token) 
        {
            pkt.args.push_back(token);
        }
        return pkt;
    }

    std::string serialize(const Packet& pkt) 
    {
        auto toString = [](PacketType t) 
            {
            switch (t)
            {
            case PacketType::REGISTER:        return "REGISTER";
            case PacketType::LOGIN:           return "LOGIN";
            case PacketType::CREATE:          return "CREATE";
            case PacketType::JOIN:            return "JOIN";
            case PacketType::MOVE:            return "MOVE";
            case PacketType::RESULT:          return "RESULT";
            case PacketType::OPPONENT_JOINED: return "OPPONENT_JOINED";
            case PacketType::START:           return "START";
            case PacketType::ERROR_PKT:       return "ERROR";
            default:                          return "UNKNOWN";
            }
            };

        std::string out = toString(pkt.type);
        for (const auto& arg : pkt.args) 
        {
            out += " " + arg;
        }
        out += "\n";
        return out;
    }

}  // namespace PacketManager