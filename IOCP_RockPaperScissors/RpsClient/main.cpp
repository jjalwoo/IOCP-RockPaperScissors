#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <conio.h>    // _getch()

#pragma comment(lib, "ws2_32.lib")

static SOCKET ConnectToServer(const std::string& ip, int port);
static bool   ClientRegister();
static bool   ClientLogin(SOCKET& outSock);

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

// =================================================================
// 게임 플레이를 동기적으로 처리하는 함수
// RESULT 메시지를 받는 순간 즉시 반환되어 상위 메뉴로 복귀
// =================================================================
void PlayGameSession(SOCKET sock)
{
    std::string line;

    // 1) 대기/시작 메시지 처리
    while (ReadLine(sock, line))
    {
        std::cout << "[서버] " << line << "\n";

        if (line == "OPPONENT_JOINED")
            std::cout << "*** 상대가 입장했습니다! ***\n";
        else if (line == "START")
        {
            std::cout << "*** 게임 시작! MOVE R/P/S 입력 ***\n";
            break;
        }
    }

    // 2) 사용자 입력 → MOVE 전송 → 서버 응답(RESULT) 대기
    while (true)
    {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;

        // MOVE 명령으로 변환
        if (input.size() == 1 &&
            (input == "R" || input == "P" || input == "S"))
        {
            input = "MOVE " + input;
        }

        // 전송 실패 시 세션 종료
        if (!SendLine(sock, input))
            return;

        // 서버로부터 RESULT를 받을 때까지 반복
        while (ReadLine(sock, line))
        {
            std::cout << "[서버] " << line << "\n";

            // RESULT 메시지를 받으면 함수 종료
            if (line.rfind("RESULT", 0) == 0)
                return;
        }
        break;
    }
}

// 회원가입 처리
bool ClientRegister()
{
    std::string username, password;

    std::cout << "\n[회원 가입]\n";
    std::cout << "ID> ";
    std::getline(std::cin, username);
    std::cout << "PW> ";
    std::getline(std::cin, password);

    SOCKET sock = ConnectToServer("127.0.0.1", 9000);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "서버 연결 실패\n";
        return false;
    }

    // REGISTER username password\n 전송
    SendLine(sock, "REGISTER " + username + " " + password);

    std::string resp;
    if (ReadLine(sock, resp))
    {
        if (resp == "ERROR DUPLICATE_USER")
        {
            std::cout << "ID가 중복되었습니다.\n\n";
        }
        else if (resp == "REGISTER_SUCCESS")
        {
            std::cout << "회원 가입이 완료되었습니다. 아무키나 입력하시면 메인메뉴로 돌아갑니다.\n";
            _getch();
        }
        else
        {
            std::cout << "가입 중 오류 발생: " << resp << "\n";
        }
    }

    closesocket(sock);
    std::cout << "\n";
    return true;
}

// 로그인 처리 (성공 시 outSock에 연결 소켓 리턴)
bool ClientLogin(SOCKET& outSock)
{
    std::string username, password;

    std::cout << "\n[로그인]\n";
    std::cout << "ID> ";
    std::getline(std::cin, username);
    std::cout << "PW> ";
    std::getline(std::cin, password);

    SOCKET sock = ConnectToServer("127.0.0.1", 9000);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "서버 연결 실패\n";
        return false;
    }

    SendLine(sock, "LOGIN " + username + " " + password);

    std::string resp;
    if (ReadLine(sock, resp))
    {
        if (resp == "LOGIN_SUCCESS")
        {
            std::cout << "로그인 성공! 게임 시작 메뉴로 이동합니다.\n\n";
            outSock = sock;
            return true;
        }
        else
        {
            std::cout << "잘못 입력했습니다. 다시 확인해주세요.\n아무키나 입력하시면 메인메뉴로 돌아갑니다.\n";
            _getch();
        }
    }

    closesocket(sock);
    std::cout << "\n";
    return false;
}


int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "WSAStartup 실패\n";
        return 1;
    }

    // 1) 메인 메뉴
    std::string input;
    while (true)
    {
        ShowMainMenu();
        std::getline(std::cin, input);

        if (input == "1")
        {
            // 로그인 시도
            SOCKET gameSock;
            if (!ClientLogin(gameSock))
            {
                continue;   // 로그인 실패 시 메인으로
            }

            // 로그인 성공하면 방 만들기/참여 루프 수행
            while (true)
            {
                ShowGameStartMenu();
                std::getline(std::cin, input);

                if (input != "1" && input != "2")
                {
                    std::cout << "[잘못된 선택]\n\n";
                    continue;
                }

                std::string roomId;
                std::cout << "\n방 ID를 입력하세요> ";
                std::getline(std::cin, roomId);

                std::string initialCmd =
                    (input == "1") ? "CREATE " + roomId
                    : "JOIN " + roomId;

                if (!SendLine(gameSock, initialCmd))
                {
                    closesocket(gameSock);
                    break;  // 서버 연결 문제 시 재로그인
                }

                std::string serverLine;
                if (!ReadLine(gameSock, serverLine))
                {
                    closesocket(gameSock);
                    break;
                }

                std::cout << "[서버] " << serverLine << "\n";
                if (serverLine.rfind("ERROR", 0) == 0)
                {
                    std::cout << "[INFO] 다시 선택하세요\n\n";
                    continue;
                }

                // 실제 게임
                PlayGameSession(gameSock);
            }

            closesocket(gameSock);
            continue;   // 메인 메뉴로

        }
        else if (input == "2")
        {
            ClientRegister();
            continue;
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

    // 2) 방 만들기/참여 및 게임 세션 루프
    while (true)
    {
        SOCKET sock = INVALID_SOCKET;
        std::string roomId, initialCmd, serverLine;

        // 2-1) CREATE/JOIN 초기 핸드셰이크
        while (true)
        {
        RETRY_GAMESTART:
            ShowGameStartMenu();
            std::getline(std::cin, input);

            if (input != "1" && input != "2")
            {
                std::cout << "[잘못된 선택]\n\n";
                continue;
            }

            std::cout << "\n방 ID를 입력하세요> ";
            std::getline(std::cin, roomId);

            initialCmd = (input == "1") ? "CREATE " + roomId
                : "JOIN " + roomId;
            sock = ConnectToServer("127.0.0.1", 9000);
            if (sock == INVALID_SOCKET)
            {
                std::cerr << "서버 연결 실패\n";
                WSACleanup();
                return 1;
            }

            if (!SendLine(sock, initialCmd) || !ReadLine(sock, serverLine))
            {
                std::cerr << "초기 명령 전송/응답 실패\n";
                closesocket(sock);
                WSACleanup();
                return 1;
            }

            std::cout << "[서버] " << serverLine << "\n";
            if (serverLine.rfind("ERROR", 0) == 0)
            {
                closesocket(sock);
                std::cout << "[INFO] 다시 선택하세요\n\n";
                goto RETRY_GAMESTART;
            }
            break;  // 방 생성/참여 성공
        }

        // 2-2) 실제 게임 플레이
        PlayGameSession(sock);

        // 2-3) 세션 정리 및 메뉴 복귀
        closesocket(sock);
        std::cout << "\n[INFO] 게임이 종료되었습니다. 다시 방 만들기/참여 메뉴로 돌아갑니다.\n";
    }

    WSACleanup();
    return 0;
}