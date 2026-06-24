#ifndef __NPC_CONVOY_AI_H
#define __NPC_CONVOY_AI_H

#include "system/SystemEntity.h"

class NPC;
class Timer;

struct ConvoyGroup {
    std::vector<NPC*> members;
    uint32 stationA, stationB;
    bool goToB;
    int8 phase; // 0=FormUp, 1=WarpOut, 2=Waiting
    Timer* timer;
    uint32 refCount;

    ConvoyGroup(uint32 a, uint32 b);
    ~ConvoyGroup();
};

class ConvoyAI {
public:
    ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx);
    ~ConvoyAI();
    void Process();

private:
    NPC* m_npc;
    ConvoyGroup* m_group;
    uint32 m_index; // 0=lead, 1=hauler, 2=tail

    GPoint GetStationPosition(uint32 stationID);
};

#endif