#ifndef __NPC_CONVOY_AI_H
#define __NPC_CONVOY_AI_H

#include "system/SystemEntity.h"

class NPC;
class Timer;

struct ConvoyGroup {
    std::vector<NPC*> members;
    uint32 stationA, stationB;
    bool goToB;
    int8 phase; // 0=FormUp, 1=Depart, 2=WarpOut, 3=Waiting
    Timer* phaseTimer;
    Timer* attackTimer; // non-null when under attack
    uint32 refCount;

    ConvoyGroup(uint32 a, uint32 b);
    ~ConvoyGroup();
    bool IsUnderAttack() const { return attackTimer != nullptr && attackTimer->Enabled(); }
    void SetAttacked();
};

class ConvoyAI {
public:
    ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx);
    ~ConvoyAI();
    void Process();

    bool IsGroupUnderAttack() const { return m_group->IsUnderAttack(); }
    void NotifyAttacked() { m_group->SetAttacked(); }

private:
    NPC* m_npc;
    ConvoyGroup* m_group;
    uint32 m_index;

    GPoint GetStationPosition(uint32 stationID);
};

#endif