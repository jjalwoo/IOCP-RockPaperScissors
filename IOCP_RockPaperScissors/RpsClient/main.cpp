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

// =================================================================
// ���� �÷��̸� ���������� ó���ϴ� �Լ�
// RESULT �޽����� �޴� ���� ��� ��ȯ�Ǿ� ���� �޴��� ����
// =================================================================
void PlayGameSession(SOCKET sock)
{
    std::string line;

    // 1) ���/���� �޽��� ó��
    while (ReadLine(sock, line))
    {
        std::cout << "[����] " << line << "\n";

        if (line == "OPPONENT_JOINED")
            std::cout << "*** ��밡 �����߽��ϴ�! ***\n";
        else if (line == "START")
        {
            std::cout << "*** ���� ����! MOVE R/P/S �Է� ***\n";
            break;
        }
    }

    // 2) ����� �Է� �� MOVE ���� �� ���� ����(RESULT) ���
    while (true)
    {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;

        // MOVE ������� ��ȯ
        if (input.size() == 1 &&
            (input == "R" || input == "P" || input == "S"))
        {
            input = "MOVE " + input;
        }

        // ���� ���� �� ���� ����
        if (!SendLine(sock, input))
            return;

        // �����κ��� RESULT�� ���� ������ �ݺ�
        while (ReadLine(sock, line))
        {
            std::cout << "[����] " << line << "\n";

            // RESULT �޽����� ������ �Լ� ����
            if (line.rfind("RESULT", 0) == 0)
                return;
        }
        break;
    }
}

// ȸ������ ó��
bool ClientRegister()
{
    std::string username, password;

    std::cout << "\n[ȸ�� ����]\n";
    std::cout << "ID> ";
    std::getline(std::cin, username);
    std::cout << "PW> ";
    std::getline(std::cin, password);

    SOCKET sock = ConnectToServer("127.0.0.1", 9000);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "���� ���� ����\n";
        return false;
    }

    // REGISTER username password\n ����
    SendLine(sock, "REGISTER " + username + " " + password);

    std::string resp;
    if (ReadLine(sock, resp))
    {
        if (resp == "ERROR DUPLICATE_USER")
        {
            std::cout << "ID�� �ߺ��Ǿ����ϴ�.\n\n";
        }
        else if (resp == "REGISTER_SUCCESS")
        {
            std::cout << "ȸ�� ������ �Ϸ�Ǿ����ϴ�. �ƹ�Ű�� �Է��Ͻø� ���θ޴��� ���ư��ϴ�.\n";
            _getch();
        }
        else
        {
            std::cout << "���� �� ���� �߻�: " << resp << "\n";
        }
    }

    closesocket(sock);
    std::cout << "\n";
    return true;
}

// �α��� ó�� (���� �� outSock�� ���� ���� ����)
bool ClientLogin(SOCKET& outSock)
{
    std::string username, password;

    std::cout << "\n[�α���]\n";
    std::cout << "ID> ";
    std::getline(std::cin, username);
    std::cout << "PW> ";
    std::getline(std::cin, password);

    SOCKET sock = ConnectToServer("127.0.0.1", 9000);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "���� ���� ����\n";
        return false;
    }

    SendLine(sock, "LOGIN " + username + " " + password);

    std::string resp;
    if (ReadLine(sock, resp))
    {
        if (resp == "LOGIN_SUCCESS")
        {
            std::cout << "�α��� ����! ���� ���� �޴��� �̵��մϴ�.\n\n";
            outSock = sock;
            return true;
        }
        else
        {
            std::cout << "�߸� �Է��߽��ϴ�. �ٽ� Ȯ�����ּ���.\n�ƹ�Ű�� �Է��Ͻø� ���θ޴��� ���ư��ϴ�.\n";
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
        std::cerr << "WSAStartup ����\n";
        return 1;
    }

    // 1) ���� �޴�
    std::string input;
    while (true)
    {
        ShowMainMenu();
        std::getline(std::cin, input);

        if (input == "1")
        {
            // �α��� �õ�
            SOCKET gameSock;
            if (!ClientLogin(gameSock))
            {
                continue;   // �α��� ���� �� ��������
            }

            // �α��� �����ϸ� �� �����/���� ���� ����
            while (true)
            {
                ShowGameStartMenu();
                std::getline(std::cin, input);

                if (input != "1" && input != "2")
                {
                    std::cout << "[�߸��� ����]\n\n";
                    continue;
                }

                std::string roomId;
                std::cout << "\n�� ID�� �Է��ϼ���> ";
                std::getline(std::cin, roomId);

                std::string initialCmd =
                    (input == "1") ? "CREATE " + roomId
                    : "JOIN " + roomId;

                if (!SendLine(gameSock, initialCmd))
                {
                    closesocket(gameSock);
                    break;  // ���� ���� ���� �� ��α���
                }

                std::string serverLine;
                if (!ReadLine(gameSock, serverLine))
                {
                    closesocket(gameSock);
                    break;
                }

                std::cout << "[����] " << serverLine << "\n";
                if (serverLine.rfind("ERROR", 0) == 0)
                {
                    std::cout << "[INFO] �ٽ� �����ϼ���\n\n";
                    continue;
                }

                // ���� ����
                PlayGameSession(gameSock);
            }

            closesocket(gameSock);
            continue;   // ���� �޴���

        }
        else if (input == "2")
        {
            ClientRegister();
            continue;
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

    // 2) �� �����/���� �� ���� ���� ����
    while (true)
    {
        SOCKET sock = INVALID_SOCKET;
        std::string roomId, initialCmd, serverLine;

        // 2-1) CREATE/JOIN �ʱ� �ڵ����ũ
        while (true)
        {
        RETRY_GAMESTART:
            ShowGameStartMenu();
            std::getline(std::cin, input);

            if (input != "1" && input != "2")
            {
                std::cout << "[�߸��� ����]\n\n";
                continue;
            }

            std::cout << "\n�� ID�� �Է��ϼ���> ";
            std::getline(std::cin, roomId);

            initialCmd = (input == "1") ? "CREATE " + roomId
                : "JOIN " + roomId;
            sock = ConnectToServer("127.0.0.1", 9000);
            if (sock == INVALID_SOCKET)
            {
                std::cerr << "���� ���� ����\n";
                WSACleanup();
                return 1;
            }

            if (!SendLine(sock, initialCmd) || !ReadLine(sock, serverLine))
            {
                std::cerr << "�ʱ� ��� ����/���� ����\n";
                closesocket(sock);
                WSACleanup();
                return 1;
            }

            std::cout << "[����] " << serverLine << "\n";
            if (serverLine.rfind("ERROR", 0) == 0)
            {
                closesocket(sock);
                std::cout << "[INFO] �ٽ� �����ϼ���\n\n";
                goto RETRY_GAMESTART;
            }
            break;  // �� ����/���� ����
        }

        // 2-2) ���� ���� �÷���
        PlayGameSession(sock);

        // 2-3) ���� ���� �� �޴� ����
        closesocket(sock);
        std::cout << "\n[INFO] ������ ����Ǿ����ϴ�. �ٽ� �� �����/���� �޴��� ���ư��ϴ�.\n";
    }

    WSACleanup();
    return 0;
}