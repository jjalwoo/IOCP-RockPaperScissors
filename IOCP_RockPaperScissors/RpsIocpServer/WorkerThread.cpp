#include "WorkerThread.h"

WorkerThread::WorkerThread()
    : m_iocpHandle(nullptr), m_threadHandle(nullptr), m_running(false) { }


WorkerThread::~WorkerThread()
{
    Stop();
}

bool WorkerThread::Start(HANDLE iocpHandle) 
{
    m_iocpHandle = iocpHandle;
    m_running = true;
    m_threadHandle = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
    return m_threadHandle != nullptr;
}

void WorkerThread::Stop() 
{
    if (!m_running) return;
    // 스레드 루프 빠져나오도록 null overlapped 패킷 전송
    PostQueuedCompletionStatus(m_iocpHandle, 0, 0, nullptr);
    WaitForSingleObject(m_threadHandle, INFINITE);
    CloseHandle(m_threadHandle);
    m_running = false;
}

DWORD WINAPI WorkerThread::ThreadProc(LPVOID lpParam) {
    auto* self = reinterpret_cast<WorkerThread*>(lpParam);
    DWORD bytesTransferred;
    ULONG_PTR key;
    OVERLAPPED* ov;

    while (true) 
    {
        BOOL ok = GetQueuedCompletionStatus(
            self->m_iocpHandle,
            &bytesTransferred,
            &key,
            &ov,
            INFINITE
        );

        // ov == nullptr 이면 Stop() 호출로 빠져나오는 신호
        if (!ok || ov == nullptr)
            break;

        // OVERLAPPED 구조체로부터 our IOContext 찾아내기
        IOContext* ctx = CONTAINING_RECORD(ov, IOContext, overlapped);

        switch (ctx->type) 
        {
        case IO_ACCEPT:
            std::cout << "[Worker] Accept 완료\n";
            // TODO: 서버 onClientAccepted(ctx) 호출
            break;

        case IO_READ:
            std::cout << "[Worker] Read 완료, 바이트: "
                << bytesTransferred << "\n";
            // TODO: 서버 onClientRead(ctx, bytesTransferred) 호출
            break;

        case IO_WRITE:
            std::cout << "[Worker] Write 완료\n";
            // TODO: 서버 onClientWrite(ctx) 호출
            break;
        }

        // TODO: ctx 재활용 또는 delete
    }

    return 0;
}
