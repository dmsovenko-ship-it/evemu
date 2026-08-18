#ifndef __EXPEDITION_MGR_H_INCL__
#define __EXPEDITION_MGR_H_INCL__

#include "utils/Singleton.h"
#include "utils/timer.h"
#include "EVE_Dungeon.h"
#include "POD_containers.h"
#include <map>

class Client;
class SystemEntity;

// PvE escalation system ("Expeditions"). Clearing an Unrated Complex / combat
// anomaly has a small chance (5%) to "escalate": the server picks a random
// system with lower security than the current one, boots it, and spawns a
// PRIVATE combat site there that only the triggering player can see (in the
// Agency / scanner). Completing each site has a 50% chance to escalate further,
// up to 4 stages. Faction chains are per the EVE University table:
//   High: Hideout -> Lookout -> Watch -> Vigil
//   Low:  Provisional Outpost -> Outpost -> Minor Annex -> Annex -> DED 6/10
//   Null: Base -> Fortress -> Military Complex -> Provincial HQ -> Fleet Staging
// (site names differ per faction; the site itself reuses the faction's DED /
// anomaly dungeon compositions so encounters stay lore-correct).
class ExpeditionMgr : public Singleton<ExpeditionMgr>
{
public:
    ExpeditionMgr();
    void Process();     // expiry cleanup (called on the 1m EntityList tick)

    // Called from NPC::Killed for any pirate NPC death in an anomaly/unrated
    // bubble. With the configured per-kill chance, starts a new expedition for
    // the killer (if they don't already have one active).
    void MaybeTrigger(Client* pKiller, uint32 factionID, uint32 sourceSystemID);

    // Called when the expedition's site is cleared (all NPCs in the site bubble
    // died). 50% chance to escalate to the next stage, else the chain ends.
    void OnSiteCleared(uint32 charID, uint32 sigItemID);

    bool HasActive(uint32 charID) const;

    // Privacy: an escalation signature is only visible to its owner. Returns
    // true if `sig` is an expedition and charID is not its owner.
    static bool IsHidden(const CosmicSignature& sig, uint32 charID)
    {
        if (sig.dungeonType != Dungeon::Type::Escalation)
            return false;
        return sig.ownerID != charID;
    }

private:
    struct Expedition {
        uint32 charID;          // owner — site is private to this pilot
        uint32 factionID;       // pirate faction of the chain
        uint32 sourceSystemID;  // where the previous site was cleared
        uint32 targetSystemID;  // where the current site lives
        uint32 sigItemID;       // current site's signature item (root anom item)
        uint8  stage;           // 1..4 (0 = not set)
        int64  expiry;          // filetime when the chain expires
    };

    // Try to escalate: roll 50%, pick a lower-security system, spawn the site.
    bool Escalate(Expedition& exp);

    // Spawn the expedition site for stage N in a random lower-sec system.
    bool SpawnStage(Expedition& exp);

    std::map<uint32, Expedition> m_expeditions;   // charID -> active expedition
    Timer m_procTimer;
};

#define sExpMgr \
    (ExpeditionMgr::get())

#endif
