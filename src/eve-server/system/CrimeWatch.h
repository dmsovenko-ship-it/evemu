#ifndef EVEMU_SERVER_CRIMEWATCH_H_
#define EVEMU_SERVER_CRIMEWATCH_H_

#include "eve-common.h"

class Client;
class NPC;

class CrimeWatch
{
public:
    CrimeWatch(Client* pClient);
    ~CrimeWatch();

    void Process();
    bool IsAggressed()      const { return m_aggressionTimer.Enabled(); }
    bool IsCriminal()       const { return m_criminalTimer.Enabled(); }
    bool HasWeaponTimer()   const { return m_weaponTimer.Enabled(); }
    uint32 GetWeaponTimerRemaining()  const { return m_weaponTimer.GetRemainingTime(); }
    uint32 GetCriminalTimerRemaining() const { return m_criminalTimer.GetRemainingTime(); }
    uint32 GetAggressionTimerRemaining() const { return m_aggressionTimer.GetRemainingTime(); }
    bool IsOutlaw()         const;
    bool IsLimitedEngagement() const { return m_limitedEngagementTimer.Enabled(); }
    bool CanDock()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool CanJump()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool IsConcordActive()  const { return m_concordTimer.Enabled() || m_concordDamageTimer.Enabled(); }

    void OnWeaponFired();
    void OnAggression(Client* pTarget, float systemSecRating);
    void OnLooting();
    void ApplyConcordPenalty();
    void SetLimitedEngagement();
    void SendAggressionChange();

protected:
    void SpawnConcordShips();
    void RespawnConcordShip(uint32 typeID);
    void ClearConcordShips();

private:
    Client* m_client;
    Timer m_aggressionTimer;
    Timer m_criminalTimer;
    Timer m_weaponTimer;
    Timer m_concordTimer;
    Timer m_concordDamageTimer;
    Timer m_concordDespawnTimer;
    Timer m_limitedEngagementTimer;
    uint32 m_concordWave;
    std::vector<float> m_concordDmgMult;
    std::vector<NPC*> m_concordShips;
};

#endif
