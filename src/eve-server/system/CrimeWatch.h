#ifndef EVEMU_SERVER_CRIMEWATCH_H_
#define EVEMU_SERVER_CRIMEWATCH_H_

#include "eve-common.h"

class Client;

class CrimeWatch
{
public:
    CrimeWatch(Client* pClient);
    ~CrimeWatch() {}

    // Called every tick from Client::Process()
    void Process();

    // Flag observers
    bool IsAggressed()      const { return m_aggressionTimer.Enabled(); }
    bool IsCriminal()       const { return m_criminalTimer.Enabled(); }
    bool HasWeaponTimer()   const { return m_weaponTimer.Enabled(); }
    bool CanDock()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled(); }
    bool CanJump()          const { return !m_aggressionTimer.Enabled(); }

    // Called when player aggresses another player
    void OnAggression(Client* pTarget, float systemSecRating);

    // CONCORD
    bool IsConcordActive()  const { return m_concordTimer.Enabled(); }

protected:
    void ApplyConcordPenalty();

private:
    Client* m_client;

    Timer m_aggressionTimer;
    Timer m_criminalTimer;
    Timer m_weaponTimer;
    Timer m_concordTimer;
};

#endif  // EVEMU_SERVER_CRIMEWATCH_H_
