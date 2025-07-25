#include "IOCPServer.h"
#include "WorkerThread.h"
#include <iostream>

bool IOCPServer::Init()
{
    // 1. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    // 2. IOCP 핸들 생성
    m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (m_iocpHandle == nullptr) 
    {
        std::cerr << "IOCP 생성 실패: " << GetLastError() << "\n";
        WSACleanup();
        return false;
    }

    // 3. 리스닝 소켓 생성 + IOCP 등록
    if (!CreateListenSocket())     // 내부에서 socket→bind→listen→CreateIoCompletionPort 처리
    {
        CloseHandle(m_iocpHandle);
        WSACleanup();
        return false;
    }

    // 4. 워커 스레드 생성
    CreateWorkerThreads();   

    return true;
}

void IOCPServer::Run()
{
    // TODO: GetQueuedCompletionStatus()를 이용한 이벤트 처리 루프

    DWORD bytes;
    ULONG_PTR key;
    OVERLAPPED* ov;

    while (true) 
    {
        BOOL ok = GetQueuedCompletionStatus( m_iocpHandle, &bytes, &key, &ov, INFINITE);

        if (!ok) 
        {
            // 오류 처리 또는 종료 조건
            break;
        }

        // key는 소켓, ov는 OVERLAPPED 구조체
        // 실제 읽기/쓰기 로직을 WorkerThread에 위임
    }

}

void IOCPServer::Shutdown()
{
    // 핸들 및 소켓 자원 해제
    CloseHandle(m_iocpHandle);
    closesocket(m_listenSocket);

    // Winsock 종료
    WSACleanup();
}

bool IOCPServer::CreateListenSocket()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        std::cerr << "소켓 생성 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9000);

    int bindResult = bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (bindResult == SOCKET_ERROR)
    {
        std::cerr << "바인드 실패: " << WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }

    int listenResult = listen(m_listenSocket, SOMAXCONN);
    if (listenResult == SOCKET_ERROR)
    {
        std::cerr << "리스닝 실패: " << WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }

    HANDLE regH = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_iocpHandle, (ULONG_PTR)m_listenSocket, 0);
    if (regH == nullptr)
    {
        std::cerr << "소켓-IOCP 연동 실패: " << GetLastError() << "\n";
        closesocket(m_listenSocket);
        return false;
    }
    return true;
}

void IOCPServer::CreateWorkerThreads()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int threadCount = sysInfo.dwNumberOfProcessors * 2; // 예시: CPU코어 * 2

    for (int i = 0; i < threadCount; ++i)
    {
        WorkerThread* worker = new WorkerThread();
        worker->Start(m_iocpHandle);
        // vector 등에 보관하면 나중에 join 또는 cleanup 시 사용 가능
    }

}



