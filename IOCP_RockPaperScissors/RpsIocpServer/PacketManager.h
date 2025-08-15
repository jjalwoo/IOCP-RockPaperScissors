#pragma once
#include <cstdint>
#include <vector>

/*
 PacketManager
  - ��Ʈ��ũ I/O�� �ְ�޴� ��Ŷ��
    ����ȭ/������ȭ�� ����Ѵ�.
*/
enum class PacketType : uint8_t
{
    AcceptRoom,
    JoinRoom,
    Move,
    Result,
    // ���߰� ����
};

struct PacketHeader
{
    PacketType m_type;   // ��Ŷ ����
    uint16_t   m_length; // payload ����
};

class PacketManager
{
public:
    /*
     Parse
      - raw buffer���� ����� payload�� �и����ش�.
      - size�� �����ϸ� false ��ȯ.
    */
    static bool Parse(
        const char* buffer,
        size_t              size,
        PacketHeader& outHeader,
        std::vector<char>& outPayload);

    /*
     Build
      - ���+payload�� �ٿ� ���ۿ� ���۸� �����.
    */
    static std::vector<char> Build(PacketType type, const std::vector<char>& payload);
};