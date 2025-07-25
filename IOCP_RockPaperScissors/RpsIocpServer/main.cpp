#include <iostream>
#include "IOCPServer.h"

#pragma comment(lib, "ws2_32.lib")

int main()
{
    IOCPServer server;

    // 1. 서버 초기화
    if (!server.Init()) 
    {
        std::cerr << "서버 초기화 실패\n";
        return -1;
    }

    std::cout << "서버가 실행되었습니다. 종료하려면 Enter 키를 누르세요...\n";

   

    // 서버 종료 신호
    server.Shutdown();    

	return 0;
}
