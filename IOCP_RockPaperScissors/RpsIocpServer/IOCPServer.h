#pragma once
#include <winsock2.h>
#include <windows.h>

class IOCPServer
{
public:
	// 서버 초기화: 소켓 생성, IOCP 핸들 생성, 워커 쓰레드 준비
	bool Init();

	// 서버 실행: IOCP 이벤트 루프 진입
	void Run();

	// 서버 종료: 핸들 및 소켓 정리
	void Shutdown();

private:
	// 리스닝 소켓 생성 및 바인딩
	bool CreateListenSocket();

	// 워커 쓰레드풀 생성
	void CreateWorkerThreads();

	HANDLE m_iocpHandle{};
	SOCKET m_listenSocket{};
};

