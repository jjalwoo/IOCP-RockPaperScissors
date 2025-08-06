#pragma once
#include <winsock2.h>
#include <string>

class Room;

/*
 Session
  - Ŭ���̾�Ʈ ���� �� ���� ��Ÿ���� ��ü.
  - ����, ����ID, �Ҽ� Room �����͸� ����� ������.
  - Send/Receive ���۸� ���� IOCPManager�� �۾� ��û.
*/
class Session
{
public:
    Session(SOCKET sock, int id);
    ~Session();

    SOCKET GetSocket() const;
    int    GetId()     const;

    void   SetRoom(Room* room);
    Room* GetRoom()   const;

    /*
     Send
      - �����͸� WSASend�� ����.
    */
    void Send(const char* data, size_t length);

    /*
     Receive
      - I/OCPManager�� WSARecv ��û.
    */
    void Receive();

private:
    SOCKET m_sock;    // Ŭ���̾�Ʈ ����
    int    m_id;      // ���� ���� ID
    Room* m_room;    // ���� ���� �� (������ nullptr)
};