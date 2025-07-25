#pragma once
#include <windows.h>
#include <thread>
#include <iostream>
#include "IOContext.h"

class WorkerThread
{
public:
    WorkerThread();
    ~WorkerThread();

    // IOCP �ڵ��� �޾Ƽ� ������ ����
    bool Start(HANDLE iocpHandle);

    void Stop();

private:
    static DWORD WINAPI ThreadProc(LPVOID lpParam);

private:
    HANDLE m_iocpHandle;
    HANDLE m_threadHandle;
    bool   m_running;

};

