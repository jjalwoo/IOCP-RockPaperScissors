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
  - Winsock �ʱ�ȭ, IOCP ����/���ε�
  - AcceptEx, WSARecv, WSASend ����Ʈ
  - GetQueuedCompletionStatus ��� ��Ŀ ������ Ǯ ����
*/
class IOCPManager
{
public:
    IOCPManager();
    ~IOCPManager();

    /*
     Initialize
      - WSAStartup, listen socket ����/���ε�/������
      - AcceptEx �Լ� �ε�, IOCP ������ listen ���� ���
      - ��Ŀ ������ �⵿
    */
    bool Initialize(uint16_t port, int workerCount);

    /*
     Run
      - ��Ŀ ��������� ��� ����� ������ ���.
      - ���ҽ� ����(����/WSACleanup).
    */
    void Run();

    /*
     ���� ���ٿ� �̱��� �ν��Ͻ�
    */
    static IOCPManager& GetInstance();

    // Accept/Recv/Send �۾� ��û
    void PostAccept();
    void PostRecv(Session* session, void* existingCtx = nullptr);
    void PostSend(Session* session, const char* data, size_t len);

private:
    static const int MAX_BUFFER = 1024;

    // I/O �۾� ����
    enum class IOOperation { Accept, Read, Write };

    // OVERLAPPED ��� I/O ���ؽ�Ʈ
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

    // ��Ŀ ������ ������
    static DWORD WINAPI WorkerThread(LPVOID lpParam);

    // ���� ó�� ����
    DWORD RunWorker();

    // I/O �Ϸ� �б�
    void HandleAccept(PerIOContext* ctx);
    void HandleRead(PerIOContext* ctx, DWORD bytes, Session* session);
    void HandleWrite(PerIOContext* ctx, Session* session);

    // ���� �� ��� ���� ����
    static void ErrorExit(const char* msg);

    // ���� ����
    IOCPManager(const IOCPManager&) = delete;
    IOCPManager& operator=(const IOCPManager&) = delete;
};