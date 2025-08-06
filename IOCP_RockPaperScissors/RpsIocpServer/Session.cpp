#include "Session.h"
#include "IOCPManager.h"

Session::Session(SOCKET sock, int id)
    : m_sock(sock)
    , m_id(id)
    , m_room(nullptr)
{
}

Session::~Session()
{
    // ������ IOCPManager�� �ݾ��ش�
}

SOCKET Session::GetSocket() const
{
    return m_sock;
}

int Session::GetId() const
{
    return m_id;
}

void Session::SetRoom(Room* room)
{
    m_room = room;
}

Room* Session::GetRoom() const
{
    return m_room;
}

void Session::Send(const char* data, size_t length)
{
    IOCPManager::GetInstance().PostSend(this, data, length);
}

void Session::Receive()
{
    IOCPManager::GetInstance().PostRecv(this);
}