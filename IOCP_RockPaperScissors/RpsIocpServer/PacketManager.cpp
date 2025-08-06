#include "PacketManager.h"
#include <cstring>

bool PacketManager::Parse( const char* buffer, size_t size, PacketHeader& outHeader, std::vector<char>& outPayload)
{
    // ��� ũ�� �˻�
    if (size < sizeof(PacketHeader))
    {
        return false;
    }

    // ��� ����
    std::memcpy(&outHeader, buffer, sizeof(PacketHeader));

    // payload ���� �˻�
    size_t bodyLen = outHeader.m_length;
    if (size < sizeof(PacketHeader) + bodyLen)
    {
        return false;
    }

    // payload �и�
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

    // ��� + payload ũ�� ��ŭ ���� �Ҵ�
    std::vector<char> buf(sizeof(PacketHeader) + payload.size());

    // ���, payload ����
    std::memcpy(buf.data(), &header, sizeof(header));
    std::memcpy(
        buf.data() + sizeof(header),
        payload.data(),
        payload.size());

    return buf;
}