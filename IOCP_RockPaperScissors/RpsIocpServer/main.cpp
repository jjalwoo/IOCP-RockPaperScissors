#include <iostream>
#include "IOCPServer.h"

#pragma comment(lib, "ws2_32.lib")

int main()
{
    IOCPServer server;

    // 1. ���� �ʱ�ȭ
    if (!server.Init()) 
    {
        std::cerr << "���� �ʱ�ȭ ����\n";
        return -1;
    }

    std::cout << "������ ����Ǿ����ϴ�. �����Ϸ��� Enter Ű�� ��������...\n";

   

    // ���� ���� ��ȣ
    server.Shutdown();    

	return 0;
}
