// server_iocp_rps.cpp
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

SOCKET g_listenSock = INVALID_SOCKET;


#define PORT            9000
#define MAX_BUFFER      1024
#define THREAD_COUNT    4

enum IO_OPERATION { IOAccept, IORead, IOWrite };

struct PER_IO_CONTEXT {
    OVERLAPPED overlapped;
    WSABUF     wsaBuf;
    char       buffer[MAX_BUFFER];
    IO_OPERATION opType;
    SOCKET      sock;
};

struct Room;
struct Session {
    SOCKET   sock;
    int      id;
    Room* room;
};

struct Room {
    std::string      id;
    Session* creator;
    Session* joiner;
    char             creatorMove;
    char             joinerMove;
    bool             started;

    Room(const std::string& rid, Session* c)
        : id(rid), creator(c),
        joiner(nullptr),
        creatorMove(0), joinerMove(0),
        started(false)
    {
    }
};

HANDLE                             g_hIOCP = NULL;
LPFN_ACCEPTEX                      g_lpfnAcceptEx = nullptr;
LPFN_GETACCEPTEXSOCKADDRS          g_lpfnGetAcceptExAddrs = nullptr;

std::vector<HANDLE>                g_workerThreads;
std::mutex                         g_sessionsLock;
std::unordered_map<SOCKET, Session*> g_sessions;
int                                g_nextSessionId = 1;

std::mutex                         g_roomsLock;
std::unordered_map<std::string, Room*> g_rooms;

void ErrorExit(const char* msg) {
    printf("%s: %d\n", msg, WSAGetLastError());
    exit(1);
}

void PostAccept(SOCKET listenSock) {
    SOCKET clientSock = WSASocket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    PER_IO_CONTEXT* ctx = new PER_IO_CONTEXT();
    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
    ctx->wsaBuf.buf = ctx->buffer;
    ctx->wsaBuf.len = MAX_BUFFER;
    ctx->opType = IOAccept;
    ctx->sock = clientSock;

    DWORD bytes = 0;
    BOOL ret = g_lpfnAcceptEx(
        listenSock, clientSock,
        ctx->buffer, 0,
        sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
        &bytes, &ctx->overlapped
    );
    if (!ret) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) ErrorExit("AcceptEx");
    }
}

void PostRecv(Session* session, PER_IO_CONTEXT* ctx = nullptr) {
    if (!ctx) {
        ctx = new PER_IO_CONTEXT();
        ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
        ctx->wsaBuf.buf = ctx->buffer;
        ctx->wsaBuf.len = MAX_BUFFER;
        ctx->opType = IORead;
        ctx->sock = session->sock;
    }

    DWORD flags = 0, bytesRecv = 0;
    int ret = WSARecv(
        session->sock, &ctx->wsaBuf, 1,
        &bytesRecv, &flags,
        &ctx->overlapped, NULL
    );
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) ErrorExit("WSARecv");
    }
}

void PostSend(Session* session, const char* data, size_t len) {
    PER_IO_CONTEXT* ctx = new PER_IO_CONTEXT();
    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
    memcpy(ctx->buffer, data, len);
    ctx->wsaBuf.buf = ctx->buffer;
    ctx->wsaBuf.len = (ULONG)len;
    ctx->opType = IOWrite;
    ctx->sock = session->sock;

    DWORD bytesSent = 0;
    int ret = WSASend(
        session->sock, &ctx->wsaBuf, 1,
        &bytesSent, 0,
        &ctx->overlapped, NULL
    );
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) ErrorExit("WSASend");
    }
}

// RPS 결과 계산:  0=무승부, 1=첫번째가 이김, -1=두번째가 이김
int ResolveRPS(char a, char b) {
    if (a == b) return 0;
    if ((a == 'R' && b == 'S') || (a == 'S' && b == 'P') || (a == 'P' && b == 'R'))
        return 1;
    return -1;
}

DWORD WINAPI WorkerThread(LPVOID) {
    DWORD       bytesTransferred;
    ULONG_PTR   key;
    OVERLAPPED* overlapped;

    while (true) {
        BOOL ok = GetQueuedCompletionStatus(
            g_hIOCP, &bytesTransferred, &key,
            &overlapped, INFINITE
        );

        Session* session = (Session*)key;
        PER_IO_CONTEXT* ctx = CONTAINING_RECORD(
            overlapped,
            PER_IO_CONTEXT,
            overlapped
        );

        if (!ok || (bytesTransferred == 0 && ctx->opType != IOAccept)) {
            // 연결 끊김 처리
            if (session) {
                std::lock_guard<std::mutex> lock(g_sessionsLock);
                closesocket(session->sock);
                g_sessions.erase(session->sock);
                delete session;
            }
            delete ctx;
            continue;
        }

        switch (ctx->opType)
        {
        case IOAccept:
        {
            setsockopt(
                ctx->sock,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (char*)&g_listenSock,
                sizeof(g_listenSock)
            );

            printf("Client connected: %d\n", ctx->sock);

            // 새 클라이언트 세션
            Session* newSession = new Session{ ctx->sock, g_nextSessionId++, nullptr };
            {
                std::lock_guard<std::mutex> lock(g_sessionsLock);
                g_sessions[newSession->sock] = newSession;
            }
            CreateIoCompletionPort((HANDLE)newSession->sock, g_hIOCP, (ULONG_PTR)newSession, 0);

            // 최초 WSARecv 요청
            PostRecv(newSession);

            // 다음 클라이언트 대기
            PostAccept((SOCKET)key);
            delete ctx;
        } break;

        case IORead: {
            // 수신된 메시지
            std::string msg(ctx->buffer, bytesTransferred);
            // 개행 문자 제거
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
                msg.pop_back();

            // 명령어와 인자 분리
            std::string cmd, arg;
            size_t sp = msg.find(' ');
            if (sp != std::string::npos) {
                cmd = msg.substr(0, sp);
                arg = msg.substr(sp + 1);
            }
            else {
                cmd = msg;
            }

            if (cmd == "CREATE") {
                std::lock_guard<std::mutex> lock(g_roomsLock);
                if (g_rooms.count(arg)) {
                    const char* err = "ERROR Room already exists\n";
                    PostSend(session, err, strlen(err));
                }
                else {
                    Room* room = new Room(arg, session);
                    session->room = room;
                    g_rooms[arg] = room;
                    const char* waiting = "Waiting for User...\n";
                    PostSend(session, waiting, strlen(waiting));
                }
            }
            else if (cmd == "JOIN") {
                std::lock_guard<std::mutex> lock(g_roomsLock);
                auto it = g_rooms.find(arg);
                if (it == g_rooms.end()) {
                    const char* err = "ERROR No such room\n";
                    PostSend(session, err, strlen(err));
                }
                else {
                    Room* room = it->second;
                    if (room->joiner) {
                        const char* err = "ERROR Room full\n";
                        PostSend(session, err, strlen(err));
                    }
                    else {
                        room->joiner = session;
                        session->room = room;
                        room->started = true;
                        // 양쪽에 상대 입장 알림
                        const char* joined = "OPPONENT_JOINED\n";
                        PostSend(room->creator, joined, strlen(joined));
                        PostSend(room->joiner, joined, strlen(joined));
                        // 게임 시작 알림
                        const char* start = "START\n";
                        PostSend(room->creator, start, strlen(start));
                        PostSend(room->joiner, start, strlen(start));
                    }
                }
            }
            else if (cmd == "MOVE") {
                Room* room = session->room;
                if (!room || !room->started) {
                    const char* err = "ERROR Game not started\n";
                    PostSend(session, err, strlen(err));
                }
                else {
                    char m = toupper(arg.empty() ? '?' : arg[0]);
                    if (session == room->creator) {
                        room->creatorMove = m;
                    }
                    else {
                        room->joinerMove = m;
                    }
                    // 모두 제출했으면 결과 계산
                    if (room->creatorMove && room->joinerMove) {
                        int res = ResolveRPS(
                            room->creatorMove,
                            room->joinerMove
                        );
                        // creator에게
                        if (res == 0) {
                            const char* r = "RESULT DRAW\n";
                            PostSend(room->creator, r, strlen(r));
                            PostSend(room->joiner, r, strlen(r));
                        }
                        else if (res == 1) {
                            const char* win = "RESULT WIN\n";
                            const char* lose = "RESULT LOSE\n";
                            PostSend(room->creator, win, strlen(win));
                            PostSend(room->joiner, lose, strlen(lose));
                        }
                        else {
                            const char* win = "RESULT WIN\n";
                            const char* lose = "RESULT LOSE\n";
                            PostSend(room->creator, lose, strlen(lose));
                            PostSend(room->joiner, win, strlen(win));
                        }
                        // 방 제거
                        std::lock_guard<std::mutex> lock(g_roomsLock);
                        g_rooms.erase(room->id);
                        delete room;
                    }
                }
            }
            else {
                // 알 수 없는 명령
                const char* err = "ERROR Unknown command\n";
                PostSend(session, err, strlen(err));
            }

            // 다음 수신 준비
            PostRecv(session, ctx);
        } break;

        case IOWrite: {
            delete ctx;
        } break;
        }
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorExit("WSAStartup");

    // 리슨 소켓
    SOCKET listenSock = WSASocket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP,
        NULL, 0, WSA_FLAG_OVERLAPPED
    );

    // AcceptEx 함수 포인터 로딩
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    GUID guidGetAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD bytes = 0;
    WSAIoctl(
        listenSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx, sizeof(guidAcceptEx),
        &g_lpfnAcceptEx, sizeof(g_lpfnAcceptEx),
        &bytes, NULL, NULL
    );
    WSAIoctl(
        listenSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAddrs, sizeof(guidGetAddrs),
        &g_lpfnGetAcceptExAddrs, sizeof(g_lpfnGetAcceptExAddrs),
        &bytes, NULL, NULL
    );

    sockaddr_in servAddr = {};
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);
    bind(listenSock, (sockaddr*)&servAddr, sizeof(servAddr));
    listen(listenSock, SOMAXCONN);

    // IOCP 생성 및 리슨 소켓 등록
    g_hIOCP = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, NULL, 0, 0
    );
    CreateIoCompletionPort(
        (HANDLE)listenSock, g_hIOCP,
        (ULONG_PTR)listenSock, 0
    );

    // 워커 스레드
    for (int i = 0; i < THREAD_COUNT; i++) {
        g_workerThreads.push_back(
            CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL)
        );
    }

    printf("IOCP RPS 서버 시작: 포트 %d\n", PORT);
    PostAccept(listenSock);

    WaitForMultipleObjects(
        (DWORD)g_workerThreads.size(),
        g_workerThreads.data(),
        TRUE, INFINITE
    );

    closesocket(listenSock);
    WSACleanup();
    return 0;
}