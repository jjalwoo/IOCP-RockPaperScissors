#include "IOCPServer.h"
#include "WorkerThread.h"
#include <iostream>

bool IOCPServer::Init()
{
    // 1. Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup ����: " << WSAGetLastError() << "\n";
        return false;
    }

    // 2. IOCP �ڵ� ����
    m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (m_iocpHandle == nullptr) 
    {
        std::cerr << "IOCP ���� ����: " << GetLastError() << "\n";
        WSACleanup();
        return false;
    }

    // 3. ������ ���� ���� + IOCP ���
    if (!CreateListenSocket())     // ���ο��� socket��bind��listen��CreateIoCompletionPort ó��
    {
        CloseHandle(m_iocpHandle);
        WSACleanup();
        return false;
    }

    // 4. ��Ŀ ������ ����
    CreateWorkerThreads();   

    return true;
}

void IOCPServer::Run()
{
    // TODO: GetQueuedCompletionStatus()�� �̿��� �̺�Ʈ ó�� ����

    DWORD bytes;
    ULONG_PTR key;
    OVERLAPPED* ov;

    while (true) 
    {
        BOOL ok = GetQueuedCompletionStatus( m_iocpHandle, &bytes, &key, &ov, INFINITE);

        if (!ok) 
        {
            // ���� ó�� �Ǵ� ���� ����
            break;
        }

        // key�� ����, ov�� OVERLAPPED ����ü
        // ���� �б�/���� ������ WorkerThread�� ����
    }

}

void IOCPServer::Shutdown()
{
    // �ڵ� �� ���� �ڿ� ����
    CloseHandle(m_iocpHandle);
    closesocket(m_listenSocket);

    // Winsock ����
    WSACleanup();
}

bool IOCPServer::CreateListenSocket()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        std::cerr << "���� ���� ����: " << WSAGetLastError() << "\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9000);

    int bindResult = bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (bindResult == SOCKET_ERROR)
    {
        std::cerr << "���ε� ����: " << WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }

    int listenResult = listen(m_listenSocket, SOMAXCONN);
    if (listenResult == SOCKET_ERROR)
    {
        std::cerr << "������ ����: " << WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }

    HANDLE regH = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_iocpHandle, (ULONG_PTR)m_listenSocket, 0);
    if (regH == nullptr)
    {
        std::cerr << "����-IOCP ���� ����: " << GetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }
    return true;
}

void IOCPServer::CreateWorkerThreads()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int threadCount = sysInfo.dwNumberOfProcessors * 2; // ����: CPU�ھ� * 2

    for (int i = 0; i < threadCount; ++i)
    {
        WorkerThread* worker = new WorkerThread();
        worker->Start(m_iocpHandle);
        // vector � �����ϸ� ���߿� join �Ǵ� cleanup �� ��� ����
    }

}



