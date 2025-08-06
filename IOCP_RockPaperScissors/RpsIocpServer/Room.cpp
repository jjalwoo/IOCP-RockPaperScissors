#include "Room.h"
#include "GameLogic.h"

Room::Room(const std::string& id, Session* creator)
    : m_id(id)
    , m_creator(creator)
    , m_joiner(nullptr)
    , m_creatorMove(0)
    , m_joinerMove(0)
    , m_started(false)
{
}

Room::~Room()
{
}

const std::string& Room::GetId() const
{
    return m_id;
}

bool Room::Join(Session* joiner)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_joiner != nullptr)
    {
        return false;
    }

    m_joiner = joiner;
    m_started = true;
    return true;
}

void Room::Leave(Session* session)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (session == m_creator)
    {
        m_creator = nullptr;
    }
    else if (session == m_joiner)
    {
        m_joiner = nullptr;
    }
}

bool Room::IsStarted() const
{
    return m_started;
}

void Room::MakeMove(Session* session, char move)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (session == m_creator)
    {
        m_creatorMove = move;
    }
    else if (session == m_joiner)
    {
        m_joinerMove = move;
    }
}

bool Room::HasBothMoves() const
{
    return m_creatorMove != 0 && m_joinerMove != 0;
}

RpsResult Room::Resolve() const
{
    int raw = GameLogic::ResolveRps(m_creatorMove, m_joinerMove);

    if (raw == 0)
    {
        return RpsResult::Draw;
    }
    else if (raw == 1)
    {
        return RpsResult::FirstWin;
    }
    else
    {
        return RpsResult::SecondWin;
    }
}

Session* Room::GetCreator() const
{
    return m_creator;
}

Session* Room::GetJoiner() const
{
    return m_joiner;
}