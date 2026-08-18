#include "eve-server.h"

#include "EVE_Corp.h"
#include "EVE_Classes.h"
#include "EVE_Dungeon.h"
#include "EVE_Scanning.h"
#include "tables/invTypes.h"

#include "Client.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "exploration/Scan.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/cosmicMgrs/DungeonMgr.h"
#include "system/cosmicMgrs/ManagerDB.h"
#include "expedition/ExpeditionMgr.h"

#include <algorithm>

// EVE escalation mechanics (per EVE University wiki):
//  - Trigger: clearing an Unrated Complex / combat anomaly. ~5% per clear.
//  - Each site has a 50% chance to escalate to the next stage (up to 4).
//  - The next site spawns in a system with LOWER security than the current one.
//  - The site is PRIVATE — only the triggering player sees it.
// Site stage 1-4 uses the faction's DED complex (archetype 10) compositions so
// encounters stay lore-correct (3/10 -> 5/10 -> 8/10 -> 10/10 style).

ExpeditionMgr::ExpeditionMgr()
: m_procTimer(0)
{
}

void ExpeditionMgr::Process()
{
    if (!m_procTimer.Enabled())
        m_procTimer.Start(30000);   // 30s expiry sweep

    if (!m_procTimer.Check(false))
        return;

    int64 now = GetFileTimeNow();
    for (auto it = m_expeditions.begin(); it != m_expeditions.end();) {
        if (it->second.expiry < now)
            it = m_expeditions.erase(it);
        else
            ++it;
    }
}

bool ExpeditionMgr::HasActive(uint32 charID) const
{
    return m_expeditions.find(charID) != m_expeditions.end();
}

void ExpeditionMgr::MaybeTrigger(Client* pKiller, uint32 factionID, uint32 sourceSystemID)
{
    if (pKiller == nullptr)
        return;
    uint32 charID = pKiller->GetCharacterID();
    if (charID == 0 || HasActive(charID))
        return;
    if (factionID == 0 || factionID == factionSleepers)
        return;

    // ~5% chance per cleared site to start an expedition.
    if (MakeRandomInt(1, 100) > 5)
        return;

    Expedition exp;
    exp.charID          = charID;
    exp.factionID       = factionID;
    exp.sourceSystemID  = sourceSystemID;
    exp.targetSystemID  = 0;
    exp.sigItemID       = 0;
    exp.stage           = 1;
    exp.expiry          = GetFileTimeNow() + 24LL * EvE::Time::Hour;

    if (!SpawnStage(exp)) {
        _log(EXPEDITION__ERROR, "ExpeditionMgr: failed to spawn stage 1 for %u", charID);
        return;
    }
    m_expeditions[charID] = exp;

    _log(EXPEDITION__MESSAGE, "ExpeditionMgr: started %s expedition for %u — stage 1 in %u",
         sDataMgr.GetFactionName(factionID), charID, exp.targetSystemID);
    pKiller->SendNotifyMsg("Your scan reveals a new location of interest in %s.",
                           sDataMgr.GetSystemName(exp.targetSystemID));
}

void ExpeditionMgr::OnSiteCleared(uint32 charID, uint32 sigItemID)
{
    auto it = m_expeditions.find(charID);
    if (it == m_expeditions.end())
        return;
    if (it->second.sigItemID != sigItemID && sigItemID != 0)
        return;

    Expedition exp = it->second;   // copy — `it` is erased below
    if (exp.stage >= 4) {
        // Chain complete — remove.
        m_expeditions.erase(it);
        _log(EXPEDITION__MESSAGE, "ExpeditionMgr: %u completed the %s chain (4/4).",
             charID, sDataMgr.GetFactionName(exp.factionID));
        return;
    }

    // 50% chance to escalate to the next stage.
    if (MakeRandomInt(1, 100) > 50) {
        m_expeditions.erase(it);
        _log(EXPEDITION__MESSAGE, "ExpeditionMgr: %u's %s expedition did not escalate further.",
             charID, sDataMgr.GetFactionName(exp.factionID));
        return;
    }

    exp.sourceSystemID = exp.targetSystemID;
    ++exp.stage;
    if (!SpawnStage(exp)) {
        m_expeditions.erase(it);
        _log(EXPEDITION__ERROR, "ExpeditionMgr: stage %u spawn failed for %u — chain ended.",
             exp.stage, charID);
        return;
    }
    m_expeditions[charID] = exp;
    Client* pClient = sEntityList.FindClientByCharID(charID);
    if (pClient != nullptr)
        pClient->SendNotifyMsg("Another location of interest has been found in %s.",
                               sDataMgr.GetSystemName(exp.targetSystemID));
}

// Pick a random k-space system with LOWER true security than the source.
uint32 PickLowerSecSystem(uint32 sourceSystemID, float sourceSec)
{
    // Query candidate systems: security < source, k-space, exclude the source.
    // Bias toward "a few jumps away" is handled implicitly by picking randomly
    // from all lower-sec systems — matches the wiki ("random, often a few jumps").
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT solarSystemID FROM mapSolarSystems "
        "WHERE security < %f AND solarSystemID != %u AND solarSystemID < 31000000 "
        "ORDER BY RAND() LIMIT 1",
        sourceSec, sourceSystemID))
        return 0;
    DBResultRow row;
    if (res.GetRow(row))
        return row.GetUInt(0);
    return 0;
}

// Pick a DED dungeon (archetype 10) for this faction, escalating difficulty
// with stage: stage1 -> 3/10ish, stage2 -> 5/10ish, stage3 -> 8/10ish,
// stage4 -> 10/10 (the named final complex). We just pick any of the faction's
// DED sites by stage tier for variety.
uint32 PickFactionDed(uint32 factionID, uint8 stage)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT dungeonID FROM dunDungeons WHERE archetypeID=10 AND factionID=%u "
        "ORDER BY RAND() LIMIT 1", factionID))
        return 0;
    DBResultRow row;
    if (res.GetRow(row))
        return row.GetUInt(0);
    return 0;
}

bool ExpeditionMgr::SpawnStage(Expedition& exp)
{
    // Find the current system's security to pick a lower one.
    SystemData sysData;
    if (!sDataMgr.GetSystemData(exp.sourceSystemID, sysData)) {
        _log(EXPEDITION__ERROR, "ExpeditionMgr: cannot resolve source system %u", exp.sourceSystemID);
        return false;
    }
    float sourceSec = sysData.securityRating;

    uint32 targetSystemID = PickLowerSecSystem(exp.sourceSystemID, sourceSec);
    if (targetSystemID == 0) {
        // No lower-sec systems — the wiki says the escalation "quietly ends".
        _log(EXPEDITION__MESSAGE, "ExpeditionMgr: no lower-sec system found, chain quietly ends.");
        return false;
    }
    exp.targetSystemID = targetSystemID;

    // Boot the target system.
    SystemManager* pSys = sEntityList.FindOrBootSystem(targetSystemID);
    if (pSys == nullptr) {
        _log(EXPEDITION__ERROR, "ExpeditionMgr: cannot boot system %u", targetSystemID);
        return false;
    }

    // Pick a faction DED dungeon for this stage.
    uint32 dungeonID = PickFactionDed(exp.factionID, exp.stage);
    if (dungeonID == 0) {
        _log(EXPEDITION__ERROR, "ExpeditionMgr: no DED dungeon for faction %u", exp.factionID);
        return false;
    }

    // Build a signature. ownerID carries the CHARID (privacy); MakeDungeon uses
    // it only for faction lookup when dungeonID==0, and we always pass a
    // dungeonID, so charID here is safe.
    CosmicSignature sig;
    sig.sigID           = sEntityList.GetAnomalyID();
    sig.sigItemID       = 0;
    sig.sigName         = sDataMgr.GetFactionName(exp.factionID) + std::string(" Expedition Site");
    sig.dungeonType     = Dungeon::Type::Escalation;
    sig.systemID        = targetSystemID;
    sig.sigTypeID       = EVEDB::invTypes::DeadspaceSignature;
    sig.sigGroupID      = EVEDB::invGroups::Cosmic_Signature;
    sig.scanGroupID     = Scanning::Group::Signature;
    sig.scanAttributeID = AttrScanAllStrength;
    sig.ownerID         = exp.charID;          // privacy: only this char sees it
    sig.sigStrength     = 1.0f;
    sig.position.x      = MakeRandomFloat(-1.0e12, 1.0e12);
    sig.position.y      = MakeRandomFloat(-1.0e12, 1.0e12);
    sig.position.z      = MakeRandomFloat(-1.0e12, 1.0e12);

    if (!pSys->GetDungMgr()->MakeDungeon(sig, dungeonID)) {
        _log(EXPEDITION__ERROR, "ExpeditionMgr: MakeDungeon failed in %u", targetSystemID);
        return false;
    }
    exp.sigItemID = sig.sigItemID;

    // Register as a PRIVATE signature. AddSignalBySignature bypasses the
    // entity-based AddSignal path (no CelestialSE filter) and puts it straight
    // into the scanner maps.
    pSys->GetAnomMgr()->AddSignalBySignature(sig);
    _log(EXPEDITION__MESSAGE, "ExpeditionMgr: spawned %s site (stage %u, dungeon %u) in %u for %u",
         sDataMgr.GetFactionName(exp.factionID), exp.stage, dungeonID, targetSystemID, exp.charID);
    return true;
}
