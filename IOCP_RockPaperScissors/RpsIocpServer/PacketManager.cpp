#include "PacketManager.h"
#include <cstring>

bool PacketManager::Parse( const char* buffer, size_t size, PacketHeader& outHeader, std::vector<char>& outPayload)
{
    // 헤더 크기 검사
    if (size < sizeof(PacketHeader))
    {
        return false;
    }

    // 헤더 복사
    std::memcpy(&outHeader, buffer, sizeof(PacketHeader));

    // payload 길이 검사
    size_t bodyLen = outHeader.m_length;
    if (size < sizeof(PacketHeader) + bodyLen)
    {
        return false;
    }

    // payload 분리
    outPayload.assign(
        buffer + sizeof(PacketHeader),
        buffer + sizeof(PacketHeader) + bodyLen);

    return true;
}

std::vector<char> PacketManager::Build(
    PacketType               type,
    const std::vector<char>& payload)
{
    PacketHeader header;
    header.m_type = type;
    header.m_length = static_cast<uint16_t>(payload.size());

    // 헤더 + payload 크기 만큼 버퍼 할당
    std::vector<char> buf(sizeof(PacketHeader) + payload.size());

    // 헤더, payload 복사
    std::memcpy(buf.data(), &header, sizeof(header));
    std::memcpy(
        buf.data() + sizeof(header),
        payload.data(),
        payload.size());

    return buf;
}