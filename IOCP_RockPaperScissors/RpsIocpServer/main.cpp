// IOCP 기반 / 방 생성·참여 · 1:1 가위바위보 서버

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

// 세션 클래스
struct Session
{

    SOCKET       sock;
    OVERLAPPED   olRecv;
    OVERLAPPED   olSend;
    WSABUF       wbufRecv;
    WSABUF       wbufSend;
    char         bufRecv[1024];
    char         bufSend[1024];
    std::string  recvData;
    int          roomId = 0;
    bool         isHost = false;
    Session* peer = nullptr;
    char         move = 0;
    bool         hasMove = false;

    Session(SOCKET s)
        : sock(s)
    {

        ZeroMemory(&olRecv, sizeof(olRecv));
        ZeroMemory(&olSend, sizeof(olSend));
    }

};

// 방 정보 구조체
struct Room
{

    Session* host = nullptr;
    Session* guest = nullptr;
};

// 전역 매칭 맵
static std::map<int, Room>      g_rooms;
static std::mutex               g_roomsMtx;
static int                      g_nextRoomId = 1;

// IOCP 핸들
static HANDLE                  g_hIocp = nullptr;

// 비동기 수신 재시작
void PostRecv(Session* s)
{

    s->wbufRecv.buf = s->bufRecv;
    s->wbufRecv.len = sizeof(s->bufRecv);

    ZeroMemory(&s->olRecv, sizeof(s->olRecv));

    DWORD flags = 0;
    DWORD bytes = 0;
    WSARecv(s->sock,
        &s->wbufRecv,
        1,
        &bytes,
        &flags,
        &s->olRecv,
        nullptr);
}

// 비동기 전송
void PostSend(Session* s,
    const std::string& msg)
{

    int len = (int)msg.size();
    memcpy(s->bufSend, msg.data(), len);

    s->wbufSend.buf = s->bufSend;
    s->wbufSend.len = len;

    ZeroMemory(&s->olSend, sizeof(s->olSend));

    DWORD sent = 0;
    WSASend(s->sock,
        &s->wbufSend,
        1,
        &sent,
        0,
        &s->olSend,
        nullptr);
}

// 가위바위보 승패 판단
const char* Judge(char a, char b)
{

    if (a == b)      return "DRAW";
    if ((a == 'R' && b == 'S') ||
        (a == 'S' && b == 'P') ||
        (a == 'P' && b == 'R'))
        return "WIN";
    return "LOSE";
}

// 명령어 처리
void HandleCommand(Session* s, const std::string& line)
{

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "CREATE")
    {

        std::lock_guard<std::mutex> lk(g_roomsMtx);
        int id = g_nextRoomId++;
        Room room;
        room.host = s;
        g_rooms[id] = room;

        s->roomId = id;
        s->isHost = true;

        PostSend(s, "ROOM " + std::to_string(id) + "\n");
    }
    else if (cmd == "JOIN")
    {

        int id;
        iss >> id;

        {
            std::lock_guard<std::mutex> lk(g_roomsMtx);
            auto it = g_rooms.find(id);

            if (it == g_rooms.end() ||
                it->second.guest)
            {
                PostSend(s, "ERROR\n");
                return;
            }

            it->second.guest = s;
            Session* h = it->second.host;
            s->peer = h;
            h->peer = s;
            s->roomId = id;
            s->isHost = false;
        }

        PostSend(s, "JOINED\n");
        PostSend(s->peer, "OPPONENT_JOINED\n");
        PostSend(s->peer, "START\n");
        PostSend(s, "START\n");
    }
    else if (cmd == "MOVE")
    {

        char mv;
        iss >> mv;

        if (!s->peer)
        {
            PostSend(s, "ERROR\n");
            return;
        }

        s->move = mv;
        s->hasMove = true;

        if (s->peer->hasMove)
        {
            const char* resMe = Judge(s->move, s->peer->move);
            const char* resPeer = Judge(s->peer->move, s->move);

            {
                std::ostringstream os;
                os << "RESULT "
                    << s->move << ' '
                    << s->peer->move << ' '
                    << resMe << "\n";
                PostSend(s, os.str());
            }
            {
                std::ostringstream os;
                os << "RESULT "
                    << s->peer->move << ' '
                    << s->move << ' '
                    << resPeer << "\n";
                PostSend(s->peer, os.str());
            }

            s->hasMove = false;
            s->peer->hasMove = false;
        }
    }
    else if (cmd == "QUIT")
    {

        // 클라이언트가 나가면 소켓 닫기
        closesocket(s->sock);
    }
    else
    {
        PostSend(s, "ERROR\n");
    }
}

// IOCP 워커 스레드
DWORD WINAPI WorkerThread(LPVOID lpParam)
{

    HANDLE       hIocp = (HANDLE)lpParam;
    DWORD        bytes = 0;
    ULONG_PTR    key = 0;
    OVERLAPPED* pol = nullptr;

    while (true)
    {
        BOOL ok = GetQueuedCompletionStatus(
            hIocp,
            &bytes,
            &key,
            &pol,
            INFINITE);

        Session* s = (Session*)key;

        if (!ok || bytes == 0)
        {
            closesocket(s->sock);
            delete s;
            continue;
        }

        if (pol == &s->olRecv)
        {
            // 수신된 데이터 누적
            s->recvData.append(s->bufRecv, bytes);

            // 한 줄씩 추출
            size_t pos;
            while ((pos = s->recvData.find('\n')) != std::string::npos)
            {
                std::string line = s->recvData.substr(0, pos);
                s->recvData.erase(0, pos + 1);

                HandleCommand(s, line);
            }

            PostRecv(s);
        }
        else  // olSend
        {
            // 전송 완료 → 아무 작업 없이 대기
        }
    }

    return 0;
}

int main()
{

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // IOCP 생성
    g_hIocp = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        nullptr,
        0,
        0);

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    for (DWORD i = 0; i < si.dwNumberOfProcessors * 2; ++i)
    {
        CreateThread(
            nullptr,
            0,
            WorkerThread,
            g_hIocp,
            0,
            nullptr);
    }

    // 리스닝 소켓
    SOCKET listenSock = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(12345);

    bind(listenSock,
        (sockaddr*)&addr,
        sizeof(addr));

    listen(listenSock, SOMAXCONN);

    std::cout << "IOCP 서버 기동 (12345)\n";

    while (true)
    {
        SOCKET client = accept(listenSock,
            nullptr,
            nullptr);

        Session* s = new Session(client);

        // IOCP에 등록
        CreateIoCompletionPort(
            (HANDLE)client,
            g_hIocp,
            (ULONG_PTR)s,
            0);

        PostRecv(s);
        PostSend(s, "WELCOME\n");

    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
