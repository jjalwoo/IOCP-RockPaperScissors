#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void ShowMainMenu()
{
    std::cout << "<���������� ����>\n";
    std::cout << "1. ���ӽ���\n";
    std::cout << "2. ȸ�� ����\n";
    std::cout << "3. ��ŷ ����\n";
    std::cout << "�����ϼ���> ";
}

void ShowGameStartMenu()
{
    std::cout << "\n<���������� ����>\n";
    std::cout << "1. �� �����\n";
    std::cout << "2. �� ����\n";
    std::cout << "�����ϼ���> ";
}

SOCKET ConnectToServer(const std::string& ip, int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(s);
        return INVALID_SOCKET;
    }

    return s;
}

bool SendLine(SOCKET s, const std::string& msg)
{
    std::string data = msg + "\n";
    return send(s, data.c_str(), (int)data.size(), 0) != SOCKET_ERROR;
}

bool ReadLine(SOCKET s, std::string& out)
{
    out.clear();
    char ch;
    while (true)
    {
        int ret = recv(s, &ch, 1, 0);
        if (ret <= 0) return false;
        if (ch == '\n') break;
        out += ch;
    }
    return true;
}

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::string input;
    std::string roomId;
    std::string initialCmd;
    SOCKET sock = INVALID_SOCKET;

    // 1) ���� �޴� ó��
    while (true)
    {
        ShowMainMenu();
        std::getline(std::cin, input);

        if (input == "1")
        {
            // ���ӽ��� ����
            ShowGameStartMenu();
            std::getline(std::cin, input);

            if (input == "1" || input == "2")
            {
                std::cout << "\n�� ID�� �Է��ϼ���> ";
                std::getline(std::cin, roomId);

                if (input == "1")
                    initialCmd = "CREATE " + roomId;
                else
                    initialCmd = "JOIN " + roomId;

                break;
            }
            else
            {
                std::cout << "[�߸��� ����]\n\n";
            }
        }
        else if (input == "2")
        {
            std::cout << "\n[ȸ�� ������ ���� ���� �����Դϴ�]\n\n";
        }
        else if (input == "3")
        {
            std::cout << "\n[��ŷ ����� ���� ���� �����Դϴ�]\n\n";
        }
        else
        {
            std::cout << "[�߸��� �����Դϴ�. �ٽ� �õ����ּ���.]\n\n";
        }
    }

    // 2) ���� ���� �� �ʱ� ��� ����
    sock = ConnectToServer("127.0.0.1", 12345);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "���� ���� ����\n";
        WSACleanup();
        return 1;
    }
    SendLine(sock, initialCmd);

    // 3) ���� ���� �񵿱� ����
    std::thread receiver([&]()
        {
            std::string line;
            while (ReadLine(sock, line))
            {
                std::cout << "[����] " << line << "\n";

                if (line == "START")
                {
                    std::cout << "*** ���� ����! MOVE R/P/S �������� �Է� ***\n";
                }
                else if (line == "OPPONENT_JOINED")
                {
                    std::cout << "*** ��밡 �����߽��ϴ�! ***\n";
                }
            }
        });
    receiver.detach();

    // 4) ����� �Է� ���� (MOVE ��� ó��)
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (input == "QUIT") break;

        if (input.size() == 1 &&
            (input == "R" || input == "P" || input == "S"))
        {
            input = "MOVE " + input;
        }

        if (!SendLine(sock, input))
        {
            std::cerr << "������ ���� ����\n";
            break;
        }
    }

    // 5) ����
    closesocket(sock);
    WSACleanup();
    return 0;
}