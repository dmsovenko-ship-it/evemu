#ifndef _EVE_SERVER_EPIC_ARC_MGR_H__
#define _EVE_SERVER_EPIC_ARC_MGR_H__

#include "../eve-server.h"

struct EpicArcData {
    uint32 arcID;
    std::string arcName;
    uint32 factionID;
    uint32 cooldownDays;
    uint32 startingAgentID;
    uint32 startingSystemID;
    uint8 level;
};

struct EpicArcMissionData {
    uint32 arcID;
    uint8 chapterNumber;
    uint8 sequenceNumber;
    uint32 missionID;
    std::string missionName;
    int8 branchID;
    uint32 rewardISK;
    float rewardStanding;
};

struct EpicArcState {
    uint32 characterID;
    uint32 arcID;
    uint8 chapterNumber;
    uint32 lastMissionID;
    int8 branchChoice;
    int64 dateStarted;
    int64 dateCompleted;
    bool completed;
};

class EpicArcMgr
: public Singleton< EpicArcMgr >
{
public:
    EpicArcMgr();
    ~EpicArcMgr();

    int Initialize();

    EpicArcData* GetArc(uint32 arcID);
    EpicArcData* GetArcByAgent(uint32 agentID);
    std::vector<EpicArcMissionData> GetChapterMissions(uint32 arcID, uint8 chapter, int8 branch = 0);
    EpicArcMissionData* GetNextMission(uint32 arcID, uint8 chapter, uint32 lastMissionID, int8 branch = 0);

    bool AgentHasArc(uint32 agentID);
    bool CanStartArc(uint32 charID, uint32 arcID);
    void StartArc(uint32 charID, uint32 arcID, uint32 agentID);
    void AdvanceMission(uint32 charID, uint32 arcID);
    void CompleteArc(uint32 charID, uint32 arcID);

private:
    void Populate();

    std::map<uint32, EpicArcData> m_arcs;
    std::map<uint32, uint32> m_agentToArc;
    std::multimap<uint64, EpicArcMissionData> m_missions;
    std::map<uint64, EpicArcState> m_state;
};

#define sEpicArcMgr \
( EpicArcMgr::get() )

#endif
