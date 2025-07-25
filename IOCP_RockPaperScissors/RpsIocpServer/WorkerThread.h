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

    // IOCP 핸들을 받아서 스레드 실행
    bool Start(HANDLE iocpHandle);

    void Stop();

private:
    static DWORD WINAPI ThreadProc(LPVOID lpParam);

private:
    HANDLE m_iocpHandle;
    HANDLE m_threadHandle;
    bool   m_running;

};

