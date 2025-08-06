#include "IOCPManager.h"

int main()
{
    // IOCPManager 인스턴스 생성
    IOCPManager server;

    // 포트 9000, 워커 스레드 4개로 초기화
    if (!server.Initialize(9000, 4))
    {
        return -1;
    }

    // 서버 실행 (워커 스레드 대기)
    server.Run();
    return 0;
}
