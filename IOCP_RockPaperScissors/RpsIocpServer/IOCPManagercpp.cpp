#include "IOCPManager.h"
#include "Session.h"
#include "Room.h"
#include "GameLogic.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// �⺻ ������: ��� �ʱ�ȭ
IOCPManager::IOCPManager()
    : m_listenSocket(INVALID_SOCKET)
    , m_hIOCP(NULL)
    , m_lpfnAcceptEx(nullptr)
    , m_lpfnGetAcceptExAddrs(nullptr)
    , m_nextSessionId(1)
{
}

IOCPManager::~IOCPManager()
{
}


// ���� ���� �޽��� ��� �� ���μ��� ��� ����
void IOCPManager::ErrorExit(const char* msg)
{
    printf("%s: %d\n", msg, WSAGetLastError());
    exit(1);
}

// �̱��� �ν��Ͻ� �ʱ�ȭ
IOCPManager* IOCPManager::s_instance = nullptr;

IOCPManager& IOCPManager::GetInstance()
{
    return *s_instance;
}

bool IOCPManager::Initialize(uint16_t port, int workerCount)
{
    // 1) Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ErrorExit("WSAStartup");
    }

    // 2) ���� ���� ���� (������ I/O��)
    m_listenSocket = WSASocket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED);

    if (m_listenSocket == INVALID_SOCKET)
    {
        ErrorExit("WSASocket");
    }

    // 3) AcceptEx, GetAcceptExAddrs �Լ� ������ �ε�
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    GUID guidGetAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD bytes = 0;

    if (WSAIoctl(
        m_listenSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx,
        sizeof(guidAcceptEx),
        &m_lpfnAcceptEx,
        sizeof(m_lpfnAcceptEx),
        &bytes,
        NULL,
        NULL) == SOCKET_ERROR)
    {
        ErrorExit("WSAIoctl(AcceptEx)");
    }

    if (WSAIoctl(
        m_listenSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAddrs,
        sizeof(guidGetAddrs),
        &m_lpfnGetAcceptExAddrs,
        sizeof(m_lpfnGetAcceptExAddrs),
        &bytes,
        NULL,
        NULL) == SOCKET_ERROR)
    {
        ErrorExit("WSAIoctl(GetAcceptExAddrs)");
    }

    // 4) ���ε� �� ����
    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    if (bind(m_listenSocket, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        ErrorExit("bind");
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        ErrorExit("listen");
    }

    // 5) IOCP ���� �� ���� ���� ���
    m_hIOCP = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        0);

    if (m_hIOCP == NULL)
    {
        ErrorExit("CreateIoCompletionPort");
    }

    if (CreateIoCompletionPort(
        (HANDLE)m_listenSocket,
        m_hIOCP,
        static_cast<ULONG_PTR>(m_listenSocket),
        0) == NULL)
    {
        ErrorExit("CreateIoCompletionPort(listen)");
    }

    // 6) ��Ŀ ������ Ǯ ����
    for (int i = 0; i < workerCount; ++i)
    {
        HANDLE hThread = CreateThread(
            NULL,
            0,
            WorkerThread,
            this,
            0,
            NULL);

        if (hThread == NULL)
        {
            ErrorExit("CreateThread");
        }

        m_workerThreads.push_back(hThread);
    }

    // �̱��� ������ ����
    s_instance = this;

    printf("IOCP RPS ���� ����: ��Ʈ %d\n", port);

    // ���� Accept ��û
    PostAccept();

    return true;
}

void IOCPManager::Run()
{
    // ��Ŀ ��������� ��� ����� ������ ���
    WaitForMultipleObjects(
        static_cast<DWORD>(m_workerThreads.size()),
        m_workerThreads.data(),
        TRUE,
        INFINITE);

    // ���ҽ� ����
    closesocket(m_listenSocket);
    WSACleanup();
}

// ��Ŀ ������ ������
DWORD WINAPI IOCPManager::WorkerThread(LPVOID lpParam)
{
    IOCPManager* self = static_cast<IOCPManager*>(lpParam);
    return self->RunWorker();
}

// ���� GetQueuedCompletionStatus ����
DWORD IOCPManager::RunWorker()
{
    DWORD       bytesTransferred;
    ULONG_PTR   key;
    OVERLAPPED* overlapped;

    while (true)
    {
        BOOL ok = GetQueuedCompletionStatus(
            m_hIOCP,
            &bytesTransferred,
            &key,
            &overlapped,
            INFINITE);

        Session* session = reinterpret_cast<Session*>(key);
        PerIOContext* ctx = CONTAINING_RECORD(
            overlapped,
            PerIOContext,
            m_overlapped);

        // ���� ���� �Ǵ� ���� ó��
        if (!ok || (bytesTransferred == 0 && ctx->m_opType != IOOperation::Accept))
        {
            if (session)
            {
                std::lock_guard<std::mutex> lock(m_sessionsMutex);
                closesocket(session->GetSocket());
                m_sessions.erase(session->GetSocket());
                delete session;
            }

            delete ctx;
            continue;
        }

        // I/O ������ �б�
        switch (ctx->m_opType)
        {
        case IOOperation::Accept:
            HandleAccept(ctx);
            break;

        case IOOperation::Read:
            HandleRead(ctx, bytesTransferred, session);
            break;

        case IOOperation::Write:
            HandleWrite(ctx, session);
            break;
        }
    }

    return 0;
}

// AcceptEx �Ϸ� �� ó��
void IOCPManager::HandleAccept(PerIOContext* ctx)
{
    // AcceptEx �� ���� ���ؽ�Ʈ ������Ʈ
    if (setsockopt(
        ctx->m_socket,
        SOL_SOCKET,
        SO_UPDATE_ACCEPT_CONTEXT,
        reinterpret_cast<char*>(&m_listenSocket),
        sizeof(m_listenSocket)) == SOCKET_ERROR)
    {
        ErrorExit("setsockopt");
    }

    printf("Client connected: %d\n", static_cast<int>(ctx->m_socket));

    // ���� ��ü ���� �� IOCP�� ���
    Session* newSession = new Session(ctx->m_socket, m_nextSessionId++);

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions[newSession->GetSocket()] = newSession;
    }

    CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(newSession->GetSocket()),
        m_hIOCP,
        reinterpret_cast<ULONG_PTR>(newSession),
        0);

    // ù WSARecv ��û
    newSession->Receive();

    // ���� Accept ��û
    PostAccept();

    delete ctx;
}

// Read �Ϸ� �� ó�� (��� �Ľ� & ����Ͻ� ����)
void IOCPManager::HandleRead(
    PerIOContext* ctx,
    DWORD         bytesTransferred,
    Session* session)
{
    // raw �����͸� ���ڿ��� ��ȯ �� ���� ����
    std::string msg(ctx->m_buffer, bytesTransferred);
    while (!msg.empty() &&
        (msg.back() == '\n' || msg.back() == '\r'))
    {
        msg.pop_back();
    }

    // ��ɾ� / ���� �и�
    std::string cmd, arg;
    size_t sep = msg.find(' ');
    if (sep != std::string::npos)
    {
        cmd = msg.substr(0, sep);
        arg = msg.substr(sep + 1);
    }
    else
    {
        cmd = msg;
    }

    // CREATE
    if (cmd == "CREATE")
    {
        std::lock_guard<std::mutex> lock(m_roomsMutex);
        if (m_rooms.count(arg))
        {
            session->Send("ERROR Room already exists\n", 27);
        }
        else
        {
            Room* room = new Room(arg, session);
            session->SetRoom(room);
            m_rooms[arg] = room;
            session->Send("Waiting for User...\n", 20);
        }
    }
    // JOIN
    else if (cmd == "JOIN")
    {
        std::lock_guard<std::mutex> lock(m_roomsMutex);
        auto it = m_rooms.find(arg);

        if (it == m_rooms.end())
        {
            session->Send("ERROR No such room\n", 19);
        }
        else
        {
            Room* room = it->second;
            if (!room->Join(session))
            {
                session->Send("ERROR Room full\n", 17);
            }
            else
            {
                session->SetRoom(room);

                // ���� �˸�
                const char* joined = "OPPONENT_JOINED\n";
                room->GetCreator()->Send(joined, 16);
                room->GetJoiner()->Send(joined, 16);

                // ���� ����
                const char* start = "START\n";
                room->GetCreator()->Send(start, 6);
                room->GetJoiner()->Send(start, 6);
            }
        }
    }
    // MOVE
    else if (cmd == "MOVE")
    {
        Room* room = session->GetRoom();
        if (!room || !room->IsStarted())
        {
            session->Send("ERROR Game not started\n", 24);
        }
        else
        {
            char m = static_cast<char>(
                std::toupper(arg.empty() ? '?' : arg[0]));
            room->MakeMove(session, m);

            if (room->HasBothMoves())
            {
                RpsResult result = room->Resolve();

                // ��� �뺸
                switch (result)
                {
                case RpsResult::Draw:
                {
                    const char* r = "RESULT DRAW\n";
                    room->GetCreator()->Send(r, 12);
                    room->GetJoiner()->Send(r, 12);
                    break;
                }
                case RpsResult::FirstWin:
                {
                    room->GetCreator()->Send("RESULT WIN\n", 11);
                    room->GetJoiner()->Send("RESULT LOSE\n", 12);
                    break;
                }
                case RpsResult::SecondWin:
                {
                    room->GetCreator()->Send("RESULT LOSE\n", 12);
                    room->GetJoiner()->Send("RESULT WIN\n", 11);
                    break;
                }
                }

                // �� ����
                std::lock_guard<std::mutex> lock(m_roomsMutex);
                m_rooms.erase(room->GetId());
                delete room;
            }
        }
    }
    // UNKNOWN
    else
    {
        session->Send("ERROR Unknown command\n", 22);
    }

    // ���� read ��û (ctx ����)
    PostRecv(session, ctx);
}

// Write �Ϸ� �� ���ؽ�Ʈ ����
void IOCPManager::HandleWrite(
    PerIOContext* ctx,
    Session*      /*session*/)
{
    delete ctx;
}

// �� Ŭ���̾�Ʈ AcceptEx ��û
void IOCPManager::PostAccept()
{
    SOCKET clientSock = WSASocket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED);

    PerIOContext* ctx = new PerIOContext();
    ZeroMemory(&ctx->m_overlapped, sizeof(OVERLAPPED));
    ctx->m_wsaBuf.buf = ctx->m_buffer;
    ctx->m_wsaBuf.len = MAX_BUFFER;
    ctx->m_opType = IOOperation::Accept;
    ctx->m_socket = clientSock;

    DWORD bytes = 0;
    BOOL  ret = m_lpfnAcceptEx(
        m_listenSocket,
        clientSock,
        ctx->m_buffer,
        0,
        sizeof(sockaddr_in) + 16,
        sizeof(sockaddr_in) + 16,
        &bytes,
        &ctx->m_overlapped);

    if (!ret)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            ErrorExit("AcceptEx");
        }
    }
}

// Ŭ���̾�Ʈ ������ ���� ��û
void IOCPManager::PostRecv(
    Session* session,
    void* existingCtx)
{
    PerIOContext* ctx = nullptr;

    if (existingCtx)
    {
        ctx = static_cast<PerIOContext*>(existingCtx);
        ZeroMemory(&ctx->m_overlapped, sizeof(OVERLAPPED));
    }
    else
    {
        ctx = new PerIOContext();
        ZeroMemory(&ctx->m_overlapped, sizeof(OVERLAPPED));
    }

    ctx->m_wsaBuf.buf = ctx->m_buffer;
    ctx->m_wsaBuf.len = MAX_BUFFER;
    ctx->m_opType = IOOperation::Read;
    ctx->m_socket = session->GetSocket();

    DWORD flags = 0;
    DWORD bytesRecv = 0;
    int   ret = WSARecv(
        session->GetSocket(),
        &ctx->m_wsaBuf,
        1,
        &bytesRecv,
        &flags,
        &ctx->m_overlapped,
        NULL);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            ErrorExit("WSARecv");
        }
    }
}

// Ŭ���̾�Ʈ�� ������ ���� ��û
void IOCPManager::PostSend(
    Session* session,
    const char* data,
    size_t      len)
{
    PerIOContext* ctx = new PerIOContext();
    ZeroMemory(&ctx->m_overlapped, sizeof(OVERLAPPED));

    memcpy(ctx->m_buffer, data, len);
    ctx->m_wsaBuf.buf = ctx->m_buffer;
    ctx->m_wsaBuf.len = static_cast<ULONG>(len);

    ctx->m_opType = IOOperation::Write;
    ctx->m_socket = session->GetSocket();

    DWORD bytesSent = 0;
    int   ret = WSASend(
        session->GetSocket(),
        &ctx->m_wsaBuf,
        1,
        &bytesSent,
        0,
        &ctx->m_overlapped,
        NULL);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            ErrorExit("WSASend");
        }
    }
}