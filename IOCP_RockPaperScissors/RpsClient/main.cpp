#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void ShowMainMenu()
{
    std::cout << "<가위바위보 게임>\n";
    std::cout << "1. 게임시작\n";
    std::cout << "2. 회원 가입\n";
    std::cout << "3. 랭킹 보기\n";
    std::cout << "선택하세요> ";
}

void ShowGameStartMenu()
{
    std::cout << "\n<가위바위보 게임>\n";
    std::cout << "1. 방 만들기\n";
    std::cout << "2. 방 참여\n";
    std::cout << "선택하세요> ";
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

    // 1) 메인 메뉴 처리
    while (true)
    {
        ShowMainMenu();
        std::getline(std::cin, input);

        if (input == "1")
        {
            // 게임시작 선택
            ShowGameStartMenu();
            std::getline(std::cin, input);

            if (input == "1" || input == "2")
            {
                std::cout << "\n방 ID를 입력하세요> ";
                std::getline(std::cin, roomId);

                if (input == "1")
                    initialCmd = "CREATE " + roomId;
                else
                    initialCmd = "JOIN " + roomId;

                break;
            }
            else
            {
                std::cout << "[잘못된 선택]\n\n";
            }
        }
        else if (input == "2")
        {
            std::cout << "\n[회원 가입은 추후 구현 예정입니다]\n\n";
        }
        else if (input == "3")
        {
            std::cout << "\n[랭킹 보기는 추후 구현 예정입니다]\n\n";
        }
        else
        {
            std::cout << "[잘못된 선택입니다. 다시 시도해주세요.]\n\n";
        }
    }

    // 2) 서버 연결 및 초기 명령 전송
    sock = ConnectToServer("127.0.0.1", 12345);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "서버 연결 실패\n";
        WSACleanup();
        return 1;
    }
    SendLine(sock, initialCmd);

    // 3) 서버 응답 비동기 수신
    std::thread receiver([&]()
        {
            std::string line;
            while (ReadLine(sock, line))
            {
                std::cout << "[서버] " << line << "\n";

                if (line == "START")
                {
                    std::cout << "*** 게임 시작! MOVE R/P/S 형식으로 입력 ***\n";
                }
                else if (line == "OPPONENT_JOINED")
                {
                    std::cout << "*** 상대가 입장했습니다! ***\n";
                }
            }
        });
    receiver.detach();

    // 4) 사용자 입력 루프 (MOVE 명령 처리)
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
            std::cerr << "서버로 전송 실패\n";
            break;
        }
    }

    // 5) 정리
    closesocket(sock);
    WSACleanup();
    return 0;
}