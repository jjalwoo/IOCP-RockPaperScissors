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
    // ������ ���� ������������ null overlapped ��Ŷ ����
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

        // ov == nullptr �̸� Stop() ȣ��� ���������� ��ȣ
        if (!ok || ov == nullptr)
            break;

        // OVERLAPPED ����ü�κ��� our IOContext ã�Ƴ���
        IOContext* ctx = CONTAINING_RECORD(ov, IOContext, overlapped);

        switch (ctx->type) 
        {
        case IO_ACCEPT:
            std::cout << "[Worker] Accept �Ϸ�\n";
            // TODO: ���� onClientAccepted(ctx) ȣ��
            break;

        case IO_READ:
            std::cout << "[Worker] Read �Ϸ�, ����Ʈ: "
                << bytesTransferred << "\n";
            // TODO: ���� onClientRead(ctx, bytesTransferred) ȣ��
            break;

        case IO_WRITE:
            std::cout << "[Worker] Write �Ϸ�\n";
            // TODO: ���� onClientWrite(ctx) ȣ��
            break;
        }

        // TODO: ctx ��Ȱ�� �Ǵ� delete
    }

    return 0;
}
