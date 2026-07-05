#include "missions/EpicArcMgr.h"
#include "database/EVEDBUtils.h"
#include "missions/MissionDB.h"

EpicArcMgr::EpicArcMgr()
{
}

EpicArcMgr::~EpicArcMgr()
{
}

int EpicArcMgr::Initialize()
{
    Populate();
    return 1;
}

void EpicArcMgr::Populate()
{
    double start = GetTimeMSeconds();

    DBQueryResult* res = new DBQueryResult();
    DBResultRow row;

    if (!sDatabase.RunQuery(*res, "SELECT arcID, arcName, factionID, cooldownDays, startingAgentID, startingSystemID, level FROM epicArc"))
        codelog(DATABASE__ERROR, "Error in EpicArcMgr query: %s", res->error.c_str());

    while (res->GetRow(row)) {
        EpicArcData data;
        data.arcID = row.GetInt(0);
        data.arcName = row.GetText(1);
        data.factionID = row.GetInt(2);
        data.cooldownDays = row.GetInt(3);
        data.startingAgentID = row.GetInt(4);
        data.startingSystemID = row.GetInt(5);
        data.level = row.GetInt(6);
        m_arcs.emplace(data.arcID, data);
        m_agentToArc.emplace(data.startingAgentID, data.arcID);
    }
    sLog.Cyan("   EpicArcMgr", "%lu Epic Arcs loaded in %.3fms.", m_arcs.size(), (GetTimeMSeconds() - start));

    start = GetTimeMSeconds();
    if (!sDatabase.RunQuery(*res, "SELECT arcID, chapterNumber, sequenceNumber, missionID, missionName, branchID, rewardISK, rewardStanding FROM epicArcMission ORDER BY arcID, chapterNumber, sequenceNumber"))
        codelog(DATABASE__ERROR, "Error in EpicArcMgr missions query: %s", res->error.c_str());

    while (res->GetRow(row)) {
        EpicArcMissionData data;
        data.arcID = row.GetInt(0);
        data.chapterNumber = row.GetInt(1);
        data.sequenceNumber = row.GetInt(2);
        data.missionID = row.GetInt(3);
        data.missionName = row.GetText(4);
        data.branchID = row.GetInt(5);
        data.rewardISK = row.GetInt(6);
        data.rewardStanding = row.GetFloat(7);
        uint64 key = (uint64(data.arcID) << 32) | (uint64(data.chapterNumber) << 16) | data.sequenceNumber;
        m_missions.emplace(key, data);
    }
    sLog.Cyan("   EpicArcMgr", "%lu Epic Arc Missions loaded in %.3fms.", m_missions.size(), (GetTimeMSeconds() - start));

    delete res;
}

EpicArcData* EpicArcMgr::GetArc(uint32 arcID)
{
    auto it = m_arcs.find(arcID);
    if (it != m_arcs.end())
        return &it->second;
    return nullptr;
}

EpicArcData* EpicArcMgr::GetArcByAgent(uint32 agentID)
{
    auto it = m_agentToArc.find(agentID);
    if (it != m_agentToArc.end())
        return GetArc(it->second);
    return nullptr;
}

bool EpicArcMgr::AgentHasArc(uint32 agentID)
{
    return m_agentToArc.find(agentID) != m_agentToArc.end();
}

bool EpicArcMgr::CanStartArc(uint32 charID, uint32 arcID)
{
    uint64 stateKey = (uint64(charID) << 32) | arcID;
    auto it = m_state.find(stateKey);
    if (it == m_state.end())
        return true;
    if (it->second.completed) {
        int64 now = GetFileTimeNow();
        int64 cooldownMs = int64(m_arcs[arcID].cooldownDays) * 24 * 60 * 60 * 10000000LL;
        if ((now - it->second.dateCompleted) >= cooldownMs)
            return true;
    }
    return false;
}

void EpicArcMgr::StartArc(uint32 charID, uint32 arcID, uint32 agentID)
{
    EpicArcState state;
    state.characterID = charID;
    state.arcID = arcID;
    state.chapterNumber = 1;
    state.lastMissionID = 0;
    state.branchChoice = 0;
    state.dateStarted = GetFileTimeNow();
    state.dateCompleted = 0;
    state.completed = false;

    uint64 stateKey = (uint64(charID) << 32) | arcID;
    m_state[stateKey] = state;

    DBerror err;
    sDatabase.RunQuery(err,
        "REPLACE INTO chrEpicArcState (characterID, arcID, chapterNumber, lastMissionID, branchChoice, dateStarted, dateCompleted, completed)"
        " VALUES (%u, %u, 1, 0, 0, %.0f, 0, 0)",
        charID, arcID, state.dateStarted);
}

void EpicArcMgr::AdvanceMission(uint32 charID, uint32 arcID)
{
    uint64 stateKey = (uint64(charID) << 32) | arcID;
    auto it = m_state.find(stateKey);
    if (it == m_state.end())
        return;

    EpicArcState& state = it->second;
    EpicArcData* arc = GetArc(arcID);
    if (arc == nullptr)
        return;

    auto range = m_missions.equal_range((uint64(arcID) << 32) | (uint64(state.chapterNumber) << 16));
    EpicArcMissionData* next = nullptr;
    uint32 highestSeq = 0;
    for (auto mit = range.first; mit != range.second; ++mit) {
        EpicArcMissionData& m = mit->second;
        if (m.sequenceNumber > state.lastMissionID % 100) {
            if (m.branchID == 0 || m.branchID == state.branchChoice) {
                if (next == nullptr || m.sequenceNumber < next->sequenceNumber) {
                    next = &m;
                }
            }
        }
        if (m.sequenceNumber > highestSeq)
            highestSeq = m.sequenceNumber;
    }

    if (next != nullptr) {
        state.lastMissionID = next->missionID;
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE chrEpicArcState SET lastMissionID = %u WHERE characterID = %u AND arcID = %u",
            next->missionID, charID, arcID);
    } else {
        // Check if chapter complete
        if (state.lastMissionID / 100 >= highestSeq) {
            state.chapterNumber++;
            if (state.chapterNumber > 7) {
                state.completed = true;
                state.dateCompleted = GetFileTimeNow();
            }
            DBerror err;
            sDatabase.RunQuery(err,
                "UPDATE chrEpicArcState SET chapterNumber = %u, completed = %u, dateCompleted = %.0f"
                " WHERE characterID = %u AND arcID = %u",
                state.chapterNumber, state.completed ? 1 : 0, state.dateCompleted, charID, arcID);
        }
    }
}

void EpicArcMgr::CompleteArc(uint32 charID, uint32 arcID)
{
    uint64 stateKey = (uint64(charID) << 32) | arcID;
    auto it = m_state.find(stateKey);
    if (it == m_state.end())
        return;

    EpicArcState& state = it->second;
    state.completed = true;
    state.dateCompleted = GetFileTimeNow();

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE chrEpicArcState SET completed = 1, dateCompleted = %.0f WHERE characterID = %u AND arcID = %u",
        state.dateCompleted, charID, arcID);
}

std::vector<EpicArcMissionData> EpicArcMgr::GetChapterMissions(uint32 arcID, uint8 chapter, int8 branch)
{
    std::vector<EpicArcMissionData> result;
    auto range = m_missions.equal_range((uint64(arcID) << 32) | (uint64(chapter) << 16));
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.branchID == 0 || it->second.branchID == branch)
            result.push_back(it->second);
    }
    return result;
}

EpicArcMissionData* EpicArcMgr::GetNextMission(uint32 arcID, uint8 chapter, uint32 lastMissionID, int8 branch)
{
    auto range = m_missions.equal_range((uint64(arcID) << 32) | (uint64(chapter) << 16));
    uint32 lastSeq = lastMissionID % 100;
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.sequenceNumber > lastSeq && (it->second.branchID == 0 || it->second.branchID == branch))
            return &it->second;
    }
    return nullptr;
}
