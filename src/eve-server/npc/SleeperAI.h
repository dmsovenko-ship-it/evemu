#ifndef EVEMU_SLEEPER_AI_H
#define EVEMU_SLEEPER_AI_H

#include "npc/NPCAI.h"

class SleeperAIMgr : public NPCAIMgr
{
public:
    SleeperAIMgr(NPC* npc);
    virtual ~SleeperAIMgr() {}

    void Process() override;
    void OnCapitalEntered();

protected:
    void RemoteRepair();
    void EnergyNeut(SystemEntity* pTarget);
    SystemEntity* FindRepairTarget();
    void CheckCapitalEscalation();

    NPC* m_pNPC;
    Timer m_remoteRepairTimer;
    Timer m_neutTimer;
    Timer m_capCheckTimer;
    uint8 m_escalationCount;

    float m_remoteRepairRange;
    float m_remoteRepairAmount;
    float m_remoteRepairDuration;
    float m_remoteRepairChance;
    float m_remoteRepairThreshold;
    uint8 m_remoteRepairMaxTargets;
    bool  m_remoteRepairArmor;
    float m_neutRange;
    float m_neutAmount;
    float m_neutDuration;
    float m_neutChance;
};

#endif
