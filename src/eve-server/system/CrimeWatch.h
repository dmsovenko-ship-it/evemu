#ifndef EVEMU_SERVER_CRIMEWATCH_H_
#define EVEMU_SERVER_CRIMEWATCH_H_

#include "eve-common.h"
#include <vector>

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
    bool IsOutlaw()         const;
    bool CanDock()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool CanJump()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool IsConcordActive()  const { return m_concordTimer.Enabled() || m_concordDamageTimer.Enabled(); }

    void OnWeaponFired();
    void OnAggression(Client* pTarget, float systemSecRating);
    void OnLooting();
    void ApplyConcordPenalty();
    void ClearSentryGuns();

protected:
    void SpawnConcordShips();
    void ClearConcordShips();
    void SpawnSentryGuns(uint32 count);
    void ApplySentryDamage();

private:
    Client* m_client;
    Timer m_aggressionTimer;
    Timer m_criminalTimer;
    Timer m_weaponTimer;
    Timer m_concordTimer;
    Timer m_concordDamageTimer;
    Timer m_sentryDamageTimer;
    std::vector<NPC*> m_concordShips;
    std::vector<NPC*> m_sentryGuns;
};

#endif
