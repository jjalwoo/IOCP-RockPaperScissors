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
        std::cerr << "서버 연결 실패\n";
        return 1;
    }

    std::string line;
    if (!ReadLine(sock, line)) return 1;
    std::cout << line << "\n"; // WELCOME

    std::cout << "서버에 연결되었습니다. 명령어를 입력하세요.\n";


    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;
        if (!SendAll(sock, line + "\n")) break;

        if (!ReadLine(sock, line)) break;
        std::cout << "[서버] " << line << "\n";

        // START, ROOM, JOINED, RESULT 등 다양하게 처리
        if (line.rfind("ROOM", 0) == 0) {
            // 방 생성 완료
        }
        else if (line == "START") {
            std::cout << "*** 게임 시작! MOVE R/P/S 형식으로 입력 ***\n";
        }
        else if (line.rfind("RESULT", 0) == 0) {
            // 예: RESULT R S WIN
        }
        else if (line.rfind("ERROR", 0) == 0) {
            // 오류 메시지
        }
        // Q(quit)는 client 쪽에서 직접 프로그램 종료
        if (line == "OPPONENT_JOINED") {
            std::cout << "*** 상대가 들어왔습니다! ***\n";
        }
        if (line == "ERROR 상대가 나갔습니다") {
            std::cout << "*** 상대가 나가 게임이 종료되었습니다 ***\n";
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
