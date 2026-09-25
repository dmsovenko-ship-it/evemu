#ifndef EVEMU_SERVER_CRIMEWATCH_H_
#define EVEMU_SERVER_CRIMEWATCH_H_

#include "eve-common.h"

class Client;
class NPC;
class SystemEntity;

// Global CONCORD strike for a non-Client criminal (a PlayerBot outlaw ganker).
// Returns the CONCORD flagship used as the kill's damage source (may be null).
SystemEntity* SpawnConcordAgainst(SystemEntity* criminalSE);

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
    // Doomsday Device leaves a 10-minute mobility cooldown: no jump drive, gate or
    // jump portal (modelled as a 10-min weapon timer — blocks dock/jump/gate).
    void OnDoomsdayFired();
    void OnAggression(Client* pTarget, float systemSecRating);
    // A player attacking a POS/structure (owned property), NOT a ship. In highsec
    // the structure is CONCORD-protected (unless its owner is war-decced): flags
    // the attacker criminal and summons CONCORD — same as aggression vs a player.
    // The defenders (tower guns / guards) are NOT flagged: they repel a criminal.
    void OnStructureAggression(uint32 structureCorpID, float systemSecRating);
    // CONCORD response to a non-Client criminal (a PlayerBot that attacked this
    // player in high-sec): spawn CONCORD and destroy the aggressor's ship.
    void RespondToBotCriminal(SystemEntity* botSE);
    // Aggression against a charbot (PlayerBot) — the charbot isn't a Client, so
    // this sets the player's aggression/criminal timer with the charbot as victim.
    void OnBotAggression(uint32 botCharID, float systemSecRating);
    // Record that someone attacked THIS player first (a Client's charID or a
    // charbot's botCharID). A player defending themselves against whoever started
    // the fight must NOT be flagged for aggression — only the initiator is.
    // Valid for the aggression window (10 min).
    void RegisterAttackBy(uint32 attackerID);
    // True if the given attacker (Client charID or charbot botCharID) started a
    // fight against this player recently — self-defence is legal, no flags.
    bool WasAttackedBy(uint32 attackerID) const
    {
        return m_attackedByTimer.Enabled() && m_attackedByID == attackerID;
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
    // The entity CONCORD is enforcing against: the bot criminal when set,
    // otherwise this client (the normal player-criminal path).
    SystemEntity* ConcordCriminal();

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
    uint32 m_attackedByID {0};
    Timer m_attackedByTimer;
    std::vector<float> m_concordDmgMult;
    std::vector<NPC*> m_concordShips;
    SystemEntity* m_concordCriminalSE {nullptr};   // non-null when enforcing against a bot
};

#endif
