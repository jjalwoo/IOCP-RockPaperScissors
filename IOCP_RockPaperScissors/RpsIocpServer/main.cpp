#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

// 방 구조체: ID, 플레이어 소켓, R/P/S 선택 저장
struct Room
{
    std::string           id;
    std::vector<SOCKET>   players;
    std::map<SOCKET, char> moves;
};

std::unordered_map<std::string, Room> rooms;
std::mutex                            roomsMutex;

// 해당 방에 속한 모든 플레이어에게 메시지 전송
void BroadcastInRoom(const Room& room)
{
    std::string data = room.id + "> ";

    // 결과 메시지는 'room.id> MSG\n' 형태로 전송
    for (auto& kv : room.moves)
    {
        // 빼고 싶다면 키 클라이언트별 메시지 커스터마이즈 가능
    }

    // 임시: 방 아이디만 붙여 보내지 않음—개별 send로 직접 전송
}

void SendToPlayer(SOCKET s, const std::string& msg)
{
    std::string data = msg + "\n";
    send(s, data.c_str(), (int)data.size(), 0);
}

// R/P/S 승부 판정
// 리턴: 'A' → 첫 번째 이동자(win), 'B' → 두 번째 이동자(win), 'D' → 무승부
char Judge(char a, char b)
{
    if (a == b)
        return 'D';

    if ((a == 'R' && b == 'S') ||
        (a == 'S' && b == 'P') ||
        (a == 'P' && b == 'R'))
    {
        return 'A';
    }

    return 'B';
}

// 클라이언트 전용 스레드 핸들러
void HandleClient(SOCKET client)
{
    // 1) 입장 전 CREATE/JOIN 명령 처리
    char    buf[256] = {};
    int     len = recv(client, buf, sizeof(buf) - 1, 0);

    if (len <= 0)
    {
        closesocket(client);
        return;
    }

    std::string cmd(buf, len);

    // 마지막 개행 제거
    if (!cmd.empty() && cmd.back() == '\n')
    {
        cmd.pop_back();
    }

    auto splitPos = cmd.find(' ');
    std::string action = (splitPos == std::string::npos)
        ? cmd
        : cmd.substr(0, splitPos);

    std::string roomId = (splitPos == std::string::npos)
        ? ""
        : cmd.substr(splitPos + 1);

    Room* roomPtr = nullptr;

    {
        std::lock_guard<std::mutex> lock(roomsMutex);

        if (action == "CREATE")
        {
            // 같은 ID의 방이 있으면 에러
            if (rooms.count(roomId))
            {
                SendToPlayer(client, "ERROR 방이 이미 존재합니다.");
                closesocket(client);
                return;
            }

            // 방 생성 후 입장
            rooms[roomId] = Room{ roomId, { client }, {} };
            roomPtr = &rooms[roomId];

            SendToPlayer(client, "WAITING 방 생성 완료, 상대를 기다리는 중입니다.");

            std::wcout << L"[서버] 방 " << roomId.c_str() << L" 생성, 대기 중\n";
        }
        else if (action == "JOIN")
        {
            // 방이 없거나 인원 초과 시 에러
            auto it = rooms.find(roomId);

            if (it == rooms.end())
            {
                SendToPlayer(client, "ERROR 해당 방이 없습니다.");
                closesocket(client);
                return;
            }

            if (it->second.players.size() >= 2)
            {
                SendToPlayer(client, "ERROR 방이 가득 찼습니다.");
                closesocket(client);
                return;
            }

            // 방 참여
            it->second.players.push_back(client);
            roomPtr = &it->second;

            SendToPlayer(client, "OK 방 참여 완료");

            std::wcout << L"[서버] 클라이언트 입장: 방 " << roomId.c_str() << L"\n";

            // 두 명 모였으니 게임 시작
            for (SOCKET p : roomPtr->players)
            {
                SendToPlayer(p, "OPPONENT_JOINED 상대가 입장했습니다!");
                SendToPlayer(p, "START 가위바위보 시작! R/P/S 중 하나를 입력하세요.");
            }
        }
        else
        {
            SendToPlayer(client, "ERROR 잘못된 명령입니다.");
            closesocket(client);
            return;
        }
    }

    // 2) 입장 후 R/P/S 선택 및 결과 처리
    while (true)
    {
        char ch;
        std::string line;

        // 한 줄 읽기
        while (true)
        {
            int ret = recv(client, &ch, 1, 0);

            if (ret <= 0)
            {
                goto CLEANUP;
            }

            if (ch == '\n')
            {
                break;
            }

            line.push_back(ch);
        }

        // MOVE 명령 인식
        if (line.rfind("MOVE ", 0) == 0 && roomPtr)
        {
            char choice = line[5];

            {
                std::lock_guard<std::mutex> lock(roomsMutex);
                roomPtr->moves[client] = choice;
            }

            SendToPlayer(client, std::string("OK 선택 입력: ") + choice);

            // 두 명이 모두 선택했는지 확인
            if (roomPtr->moves.size() == 2)
            {
                SOCKET p1 = roomPtr->players[0];
                SOCKET p2 = roomPtr->players[1];
                char   m1 = roomPtr->moves[p1];
                char   m2 = roomPtr->moves[p2];

                char result = Judge(m1, m2);

                // 결과 메시지 작성
                std::string msg1;
                std::string msg2;

                if (result == 'D')
                {
                    msg1 = "RESULT 무승부! 당신: ";
                    msg2 = "RESULT 무승부! 당신: ";
                }
                else if (result == 'A')
                {
                    msg1 = "RESULT 승리! 당신: ";
                    msg2 = "RESULT 패배! 당신: ";
                }
                else // 'B'
                {
                    msg1 = "RESULT 패배! 당신: ";
                    msg2 = "RESULT 승리! 당신: ";
                }

                // "RESULT 승리! 당신: R 상대: S" 형태
                msg1 += m1;
                msg1 += " 상대: ";
                msg1 += m2;

                msg2 += m2;
                msg2 += " 상대: ";
                msg2 += m1;

                // 양쪽 클라이언트에 전송
                SendToPlayer(p1, msg1);
                SendToPlayer(p2, msg2);

                // 다음 라운드를 위해 선택 기록 삭제
                {
                    std::lock_guard<std::mutex> lock(roomsMutex);
                    roomPtr->moves.clear();
                }

                // 다시 START 메시지로 재시작 안내
                for (SOCKET p : roomPtr->players)
                {
                    SendToPlayer(p, "START 다음 라운드 시작! R/P/S 입력하세요.");
                }
            }
        }
        else
        {
            // 그 외 메시지는 방 내 전체에 브로드캐스트
            std::lock_guard<std::mutex> lock(roomsMutex);

            for (SOCKET p : roomPtr->players)
            {
                if (p != client)
                {
                    SendToPlayer(p, "CHAT " + line);
                }
            }
        }
    }

CLEANUP:

    // 클라이언트 연결 종료 처리
    {
        std::lock_guard<std::mutex> lock(roomsMutex);

        if (roomPtr)
        {
            auto& vec = roomPtr->players;
            vec.erase(std::remove(vec.begin(), vec.end(), client),
                vec.end());

            // 방이 비면 삭제
            if (vec.empty())
            {
                rooms.erase(roomPtr->id);
                std::wcout << L"[서버] 방 " << roomId.c_str() << L" 삭제\n";
            }
        }
    }

    closesocket(client);
}

int main()
{
    WSADATA wsa;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(12345);

    bind(listener, (sockaddr*)&addr, sizeof(addr));
    listen(listener, SOMAXCONN);

    std::cout << "[RSP GAME 서버 시작]..."<< std::endl;

    while (true)
    {
        SOCKET client = accept(listener, nullptr, nullptr);

        if (client == INVALID_SOCKET)
        {
            continue;
        }

        std::cout << "[서버] 클라이언트 연결 socket: " << client << std::endl;

        std::thread(HandleClient, client).detach();
    }

    closesocket(listener);
    WSACleanup();

    return 0;
}