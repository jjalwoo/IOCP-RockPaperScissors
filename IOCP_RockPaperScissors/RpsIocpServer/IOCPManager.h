#pragma once
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#pragma comment(lib, "Ws2_32.lib")


class Session;
class Room;

/*
 IOCPManager
  - Winsock 초기화, IOCP 생성/바인딩
  - AcceptEx, WSARecv, WSASend 포스트
  - GetQueuedCompletionStatus 기반 워커 스레드 풀 관리
*/
class IOCPManager
{
public:
    IOCPManager();
    ~IOCPManager();

    /*
     Initialize
      - WSAStartup, listen socket 생성/바인딩/리스닝
      - AcceptEx 함수 로딩, IOCP 생성과 listen 소켓 등록
      - 워커 스레드 기동
    */
    bool Initialize(uint16_t port, int workerCount);

    /*
     Run
      - 워커 스레드들이 모두 종료될 때까지 대기.
      - 리소스 정리(소켓/WSACleanup).
    */
    void Run();

    /*
     전역 접근용 싱글톤 인스턴스
    */
    static IOCPManager& GetInstance();

    // Accept/Recv/Send 작업 요청
    void PostAccept();
    void PostRecv(Session* session, void* existingCtx = nullptr);
    void PostSend(Session* session, const char* data, size_t len);

private:
    static const int MAX_BUFFER = 1024;

    // I/O 작업 구분
    enum class IOOperation { Accept, Read, Write };

    // OVERLAPPED 기반 I/O 컨텍스트
    struct PerIOContext
    {
        OVERLAPPED  m_overlapped;
        WSABUF      m_wsaBuf;
        char        m_buffer[MAX_BUFFER];
        IOOperation m_opType;
        SOCKET      m_socket;
    };

    SOCKET                                 m_listenSocket;
    HANDLE                                 m_hIOCP;
    LPFN_ACCEPTEX                          m_lpfnAcceptEx;
    LPFN_GETACCEPTEXSOCKADDRS              m_lpfnGetAcceptExAddrs;

    std::vector<HANDLE>                    m_workerThreads;

    std::mutex                             m_sessionsMutex;
    std::unordered_map<SOCKET, Session*>   m_sessions;
    int                                    m_nextSessionId;

    std::mutex                             m_roomsMutex;
    std::unordered_map<std::string, Room*> m_rooms;

    static IOCPManager* s_instance;

    // 워커 스레드 진입점
    static DWORD WINAPI WorkerThread(LPVOID lpParam);

    // 실제 처리 루프
    DWORD RunWorker();

    // I/O 완료 분기
    void HandleAccept(PerIOContext* ctx);
    void HandleRead(PerIOContext* ctx, DWORD bytes, Session* session);
    void HandleWrite(PerIOContext* ctx, Session* session);

    // 오류 시 즉시 종료 헬퍼
    static void ErrorExit(const char* msg);

    // 복사 금지
    IOCPManager(const IOCPManager&) = delete;
    IOCPManager& operator=(const IOCPManager&) = delete;
};