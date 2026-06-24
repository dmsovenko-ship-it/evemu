#ifndef __NPC_CONVOY_AI_H
#define __NPC_CONVOY_AI_H

#include "system/SystemEntity.h"

class NPC;
class SystemManager;
class Timer;

class ConvoyAI {
public:
    ConvoyAI(NPC* who, uint32 stationA, uint32 stationB);
    ~ConvoyAI();

    void Process();

protected:
    void WarpToStation(uint32 stationID);
    void WarpComplete();

private:
    enum State { Idle, Warping, Waiting };

    NPC* m_npc;
    State m_state;
    uint32 m_stationA;
    uint32 m_stationB;
    bool m_goingToB;
    Timer* m_waitTimer;
};

#endif
