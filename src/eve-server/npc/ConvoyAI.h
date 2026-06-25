#ifndef __NPC_CONVOY_AI_H
#define __NPC_CONVOY_AI_H

#include "system/SystemEntity.h"

class NPC;
class Timer;

struct ConvoyGroup {
    std::vector<NPC*> members;
    uint32 stationA, stationB;
    bool goToB;
    bool sameCorp; // both stations belong to same corporation
    int8 phase; // 0=FormUp, 1=Departure, 2=Warping, 3=Waiting
    Timer* phaseTimer;
    Timer* attackTimer;
    uint32 refCount;

    ConvoyGroup(uint32 a, uint32 b, bool sameCorpFlag);
    ~ConvoyGroup();
    bool IsUnderAttack() const { return attackTimer != nullptr && attackTimer->Enabled(); }
    void SetAttacked(SystemEntity* attacker);
    void WakeUpAll(SystemEntity* attacker);
};

class ConvoyAI {
public:
    ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx);
    ~ConvoyAI();
    void Process();

    bool IsGroupUnderAttack() const { return m_group->IsUnderAttack(); }
    void NotifyAttacked(SystemEntity* attacker) { m_group->SetAttacked(attacker); }

private:
    NPC* m_npc;
    ConvoyGroup* m_group;
    uint32 m_index;
    Timer* m_startTimer; // staggered departure delay

    GPoint GetStationPosition(uint32 stationID);
    GPoint GetDeparturePoint();
};

#endif