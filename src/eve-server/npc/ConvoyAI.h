#ifndef __NPC_CONVOY_AI_H
#define __NPC_CONVOY_AI_H

#include "system/SystemEntity.h"

class NPC;
class Timer;

struct ConvoyGroup {
    std::vector<NPC*> members;
    uint32 stationA, stationB;
    uint32 destSystemID;        // 0 = same-system route, >0 = cross-system route target
    uint32 sourceGateID;        // stargate to jump from in source system
    uint32 destGateID;          // stargate to arrive at in destination system
    bool goToB;
    bool sameCorp; // both stations belong to same corporation
    int8 phase; // 0=FormUp, 1=Departure, 2=Warping, 3=Waiting, 4=GateJump
    uint8 factionID;
    Timer* phaseTimer;
    Timer* attackTimer;
    Timer* transitTimer;        // cross-system transit countdown
    uint32 refCount;

    ConvoyGroup(uint32 a, uint32 b, bool sameCorpFlag);
    ~ConvoyGroup();
    bool IsUnderAttack() const { return attackTimer != nullptr && attackTimer->Enabled(); }
    void SetAttacked(SystemEntity* attacker);
    void WakeUpAll(SystemEntity* attacker);
    bool IsCrossSystem() const { return destSystemID != 0; }
};

class CivilianMgr;

class ConvoyAI {
public:
    ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx);
    ~ConvoyAI();
    void Process();

    bool IsGroupUnderAttack() const { return m_group->IsUnderAttack(); }
    void NotifyAttacked(SystemEntity* attacker) { m_group->SetAttacked(attacker); }
    uint32 GetStationA() const { return m_group->stationA; }
    uint32 GetStationB() const { return m_group->stationB; }
    bool GetSameCorp() const { return m_group->sameCorp; }
    ConvoyGroup* GetGroup() const { return m_group; }

private:
    NPC* m_npc;
    ConvoyGroup* m_group;
    uint32 m_index;
    Timer* m_startTimer; // staggered departure delay
    bool m_transferRequested;    // true if cross-system transfer has been initiated

    GPoint GetStationPosition(uint32 stationID);
    GPoint GetDeparturePoint();
};

#endif