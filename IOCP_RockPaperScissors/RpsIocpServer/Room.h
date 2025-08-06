#pragma once
#include <string>
#include <mutex>

class Session;
enum class RpsResult;

/*
 Room
  - �� ���� �� �ϳ��� ����.
  - creator/joiner, �� �÷��̾��� ���ð�, �� ���� ����.
  - Join, MakeMove, Resolve ���� API ����.
*/
class Room
{
public:
    Room(const std::string& id, Session* creator);
    ~Room();

    const std::string& GetId() const;

    /*
     Join
      - �濡 ������ �߰�.
      - �̹� �����ڰ� ������ false.
    */
    bool Join(Session* joiner);

    /*
     Leave
      - �濡�� ���� ����.
    */
    void Leave(Session* session);

    bool IsStarted() const;

    /*
     MakeMove
      - creator/joiner �� ���� ȣ���ߴ��� ������ move ����.
    */
    void MakeMove(Session* session, char move);

    /*
     HasBothMoves
      - ���� �� MOVE ����� ���´��� Ȯ��.
    */
    bool HasBothMoves() const;

    /*
     Resolve
      - GameLogic�� ȣ���� ���� ��� ���.
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