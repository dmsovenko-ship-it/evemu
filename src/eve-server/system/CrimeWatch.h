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
    uint32 GetAggressionTargetID()   const { return m_aggressionTargetID; }
    bool IsOutlaw()         const;
    bool IsLimitedEngagement() const { return m_limitedEngagementTimer.Enabled(); }
    bool CanDock()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool CanJump()          const { return !m_aggressionTimer.Enabled() && !m_weaponTimer.Enabled() && !IsOutlaw(); }
    bool IsConcordActive()  const { return m_concordTimer.Enabled() || m_concordDamageTimer.Enabled(); }

    void OnWeaponFired();
    void OnAggression(Client* pTarget, float systemSecRating);
    // Aggression against a charbot (PlayerBot) — the charbot isn't a Client, so
    // this sets the player's aggression/criminal timer with the charbot as victim.
    void OnBotAggression(uint32 botCharID, float systemSecRating);
    // Record that a charbot attacked THIS player first. A player defending
    // themselves against a charbot that started the fight must NOT be flagged
    // for aggression — only the initiator is. Valid for 10 minutes (matching the
    // aggression window).
    void RegisterBotAttack(uint32 botCharID)
    {
        m_attackedByBotID = botCharID;
        m_attackedByBotTimer.Start(600000);
    }
    // True if the given charbot started a fight against this player recently.
    bool WasAttackedByBot(uint32 botCharID) const
    {
        return m_attackedByBotTimer.Enabled() && m_attackedByBotID == botCharID;
    }
    void OnProbeLaunch();
    void OnLooting();
    void ApplyConcordPenalty();
    void SetLimitedEngagement();
    void SendAggressionChange();
    void UpdateSessionChangeTimer();
    // True while aggression/weapon/criminal timers are still running — used so the
    // session-change timer does not clear the aggression cooldown early.
    bool HasActiveTimers() const {
        return m_aggressionTimer.Enabled() || m_weaponTimer.Enabled() || m_criminalTimer.Enabled();
    }

protected:
    void SpawnConcordShips();
    void RespawnConcordShip(uint32 typeID);
    void ClearConcordShips();

private:
    Client* m_client;
    uint32 m_aggressionTargetID {0};
    Timer m_aggressionTimer;
    Timer m_criminalTimer;
    Timer m_weaponTimer;
    Timer m_concordTimer;
    Timer m_concordDamageTimer;
    Timer m_concordDespawnTimer;
    Timer m_limitedEngagementTimer;
    uint32 m_concordWave;
    uint32 m_attackedByBotID {0};
    Timer m_attackedByBotTimer;
    std::vector<float> m_concordDmgMult;
    std::vector<NPC*> m_concordShips;
};

#endif
