#pragma once
#include <cstdint>
#include <vector>

/*
 PacketManager
  - 네트워크 I/O로 주고받는 패킷의
    직렬화/역직렬화를 담당한다.
*/
enum class PacketType : uint8_t
{
    AcceptRoom,
    JoinRoom,
    Move,
    Result,
    // …추가 가능
};

struct PacketHeader
{
    PacketType m_type;   // 패킷 종류
    uint16_t   m_length; // payload 길이
};

class PacketManager
{
public:
    /*
     Parse
      - raw buffer에서 헤더와 payload를 분리해준다.
      - size가 부족하면 false 반환.
    */
    static bool Parse(
        const char* buffer,
        size_t              size,
        PacketHeader& outHeader,
        std::vector<char>& outPayload);

    /*
     Build
      - 헤더+payload를 붙여 전송용 버퍼를 만든다.
    */
    static std::vector<char> Build(PacketType type, const std::vector<char>& payload);
};