#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

bool ReadLine(SOCKET s, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        int ret = recv(s, &ch, 1, 0);
        if (ret <= 0) return false;
        if (ch == '\n') break;
        out.push_back(ch);
    }
    return true;
}

bool SendAll(SOCKET s, const std::string& msg) {
    const char* ptr = msg.c_str();
    int remaining = (int)msg.size();
    while (remaining > 0) {
        int sent = send(s, ptr, remaining, 0);
        if (sent == SOCKET_ERROR || sent == 0) return false;
        ptr += sent;
        remaining -= sent;
    }
    return true;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serv = {};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    if (connect(sock, (sockaddr*)&serv, sizeof(serv)) != 0) {
        std::cerr << "���� ���� ����\n";
        return 1;
    }

    std::string line;
    if (!ReadLine(sock, line)) return 1;
    std::cout << line << "\n"; // WELCOME

    std::cout << "������ ����Ǿ����ϴ�. ��ɾ �Է��ϼ���.\n";


    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;
        if (!SendAll(sock, line + "\n")) break;

        if (!ReadLine(sock, line)) break;
        std::cout << "[����] " << line << "\n";

        // START, ROOM, JOINED, RESULT �� �پ��ϰ� ó��
        if (line.rfind("ROOM", 0) == 0) {
            // �� ���� �Ϸ�
        }
        else if (line == "START") {
            std::cout << "*** ���� ����! MOVE R/P/S �������� �Է� ***\n";
        }
        else if (line.rfind("RESULT", 0) == 0) {
            // ��: RESULT R S WIN
        }
        else if (line.rfind("ERROR", 0) == 0) {
            // ���� �޽���
        }
        // Q(quit)�� client �ʿ��� ���� ���α׷� ����
        if (line == "OPPONENT_JOINED") {
            std::cout << "*** ��밡 ���Խ��ϴ�! ***\n";
        }
        if (line == "ERROR ��밡 �������ϴ�") {
            std::cout << "*** ��밡 ���� ������ ����Ǿ����ϴ� ***\n";
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
