#ifndef EVEMU_SERVER_CRIMEWATCH_H_
#define EVEMU_SERVER_CRIMEWATCH_H_

#include "eve-common.h"

class Client;
class NPC;

class CrimeWatch
{
public:
    CrimeWatch(Client* pClient);
    ~CrimeWatch() {}

    void Process();
    bool IsAggressed()      const { return m_aggressionTimer.Enabled(); }
    bool IsCriminal()       const { return m_criminalTimer.Enabled(); }
    bool HasWeaponTimer()   const { return m_weaponTimer.Enabled(); }
    bool CanDock()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled(); }
    bool CanJump()          const { return !m_aggressionTimer.Enabled(); }
    bool IsConcordActive()  const { return m_concordTimer.Enabled(); }

    void OnAggression(Client* pTarget, float systemSecRating);
    void ApplyConcordPenalty();

protected:
    void SpawnConcordShips();

private:
    Client* m_client;
    Timer m_aggressionTimer;
    Timer m_criminalTimer;
    Timer m_weaponTimer;
    Timer m_concordTimer;
    std::vector<NPC*> m_concordShips;
};

#endif  // EVEMU_SERVER_CRIMEWATCH_H_
