#pragma once
#include <winsock2.h>
#include <windows.h>

class IOCPServer
{
public:
	// ���� �ʱ�ȭ: ���� ����, IOCP �ڵ� ����, ��Ŀ ������ �غ�
	bool Init();

	// ���� ����: IOCP �̺�Ʈ ���� ����
	void Run();

	// ���� ����: �ڵ� �� ���� ����
	void Shutdown();

private:
	// ������ ���� ���� �� ���ε�
	bool CreateListenSocket();

	// ��Ŀ ������Ǯ ����
	void CreateWorkerThreads();

	HANDLE m_iocpHandle{};
	SOCKET m_listenSocket{};
};

