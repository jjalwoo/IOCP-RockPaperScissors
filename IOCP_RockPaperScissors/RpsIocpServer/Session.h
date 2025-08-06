#pragma once
#include <winsock2.h>
#include <string>

class Room;

/*
 Session
  - 클라이언트 연결 한 건을 나타내는 객체.
  - 소켓, 세션ID, 소속 Room 포인터를 멤버로 가진다.
  - Send/Receive 래퍼를 통해 IOCPManager에 작업 요청.
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
      - 데이터를 WSASend로 보냄.
    */
    void Send(const char* data, size_t length);

    /*
     Receive
      - I/OCPManager에 WSARecv 요청.
    */
    void Receive();

private:
    SOCKET m_sock;    // 클라이언트 소켓
    int    m_id;      // 세션 고유 ID
    Room* m_room;    // 참가 중인 방 (없으면 nullptr)
};