#pragma once
#include <string>
#include <mutex>

class Session;
enum class RpsResult;

/*
 Room
  - 한 게임 방 하나를 관리.
  - creator/joiner, 두 플레이어의 선택값, 룸 상태 보유.
  - Join, MakeMove, Resolve 등의 API 제공.
*/
class Room
{
public:
    Room(const std::string& id, Session* creator);
    ~Room();

    const std::string& GetId() const;

    /*
     Join
      - 방에 참가자 추가.
      - 이미 참가자가 있으면 false.
    */
    bool Join(Session* joiner);

    /*
     Leave
      - 방에서 세션 제거.
    */
    void Leave(Session* session);

    bool IsStarted() const;

    /*
     MakeMove
      - creator/joiner 중 누가 호출했는지 구분해 move 저장.
    */
    void MakeMove(Session* session, char move);

    /*
     HasBothMoves
      - 양쪽 다 MOVE 명령을 보냈는지 확인.
    */
    bool HasBothMoves() const;

    /*
     Resolve
      - GameLogic을 호출해 최종 결과 계산.
    */
    RpsResult Resolve() const;

    Session* GetCreator() const;
    Session* GetJoiner()  const;

private:
    std::string m_id;
    Session* m_creator;
    Session* m_joiner;
    char        m_creatorMove;
    char        m_joinerMove;
    bool        m_started;
    mutable std::mutex m_mutex;
};