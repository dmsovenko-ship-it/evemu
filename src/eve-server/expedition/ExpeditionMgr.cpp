#include "eve-server.h"

#include "EVE_Corp.h"
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

bool ExpeditionMgr::GetExpedition(uint32 charID, ExpeditionView& out) const
{
    auto it = m_expeditions.find(charID);
    if (it == m_expeditions.end())
        return false;
    const Expedition& exp = it->second;
    out.instanceID   = exp.sigItemID;
    out.solarSystemID = exp.targetSystemID;
    out.creationTime = exp.expiry - 24LL * EvE::Time::Hour;
    out.expiryTime   = exp.expiry;
    out.factionID    = exp.factionID;
    out.stage        = exp.stage;
    return true;
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
    pKiller->SendNotifyMsg("Your scan reveals a %s: %s.",
                           ExpeditionName(factionID, 1).c_str(),
                           sDataMgr.GetSystemName(exp.targetSystemID));
    // Refresh the client's Journal "Expeditions" tab.
    PyTuple* noti = new PyTuple(1);
        noti->SetItem(0, new PyLong(exp.sigItemID));
    pKiller->SendNotification("OnEscalatingPathChange", "charid", &noti);
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
    if (pClient != nullptr) {
        pClient->SendNotifyMsg("Another stage of the %s found in %s.",
                               ExpeditionName(exp.factionID, exp.stage).c_str(),
                               sDataMgr.GetSystemName(exp.targetSystemID));
        PyTuple* noti = new PyTuple(1);
            noti->SetItem(0, new PyLong(exp.sigItemID));
        pClient->SendNotification("OnEscalatingPathChange", "charid", &noti);
    }
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

// Expedition chain names per EVE University (faction x stage). The site itself
// reuses the faction's DED complex (3/10 -> 5/10 -> 8/10-ish -> 10/10-ish).
std::string ExpeditionMgr::ExpeditionName(uint32 factionID, uint8 stage)
{
    switch (factionID) {
        case factionAngel: {
            switch (stage) {
                case 1: return "Angel Lookout";
                case 2: return "Angel Watch";
                case 3: return "Angel Annex";
                case 4: return "Angel Military Complex";
            } break;
        }
        case factionBloodRaider: {
            switch (stage) {
                case 1: return "Blood Lookout";
                case 2: return "Blood Watch";
                case 3: return "Blood Annex";
                case 4: return "Blood Military Complex";
            } break;
        }
        case factionGuristas: {
            switch (stage) {
                case 1: return "Guristas Lookout";
                case 2: return "Guristas Watch";
                case 3: return "Guristas Annex";
                case 4: return "Guristas Military Complex";
            } break;
        }
        case factionSanshas: {
            switch (stage) {
                case 1: return "Sansha Lookout";
                case 2: return "Sansha Watch";
                case 3: return "Sansha Annex";
                case 4: return "Sansha Military Complex";
            } break;
        }
        case factionSerpentis: {
            switch (stage) {
                case 1: return "Serpentis Lookout";
                case 2: return "Serpentis Watch";
                case 3: return "Serpentis Annex";
                case 4: return "Serpentis Military Complex";
            } break;
        }
        case factionRogueDrones: {
            switch (stage) {
                case 1: return "Rogue Drone Lookout";
                case 2: return "Rogue Drone Watch";
                case 3: return "Rogue Drone Annex";
                case 4: return "Rogue Drone Military Complex";
            } break;
        }
    }
    return "Expedition Site";
}

uint32 ExpeditionMgr::ExpeditionDungeon(uint32 factionID, uint8 stage)
{
    // DED complexes (archetype 10) per faction. Stage maps to difficulty:
    // 1 -> 3/10, 2 -> 5/10, 3+ -> the faction's hard/final complex.
    switch (factionID) {
        case factionAngel: {
            switch (stage) {
                case 1: return 2300;   // Angel Repurposed Outpost
                case 2: return 2600;   // Angel 5/10 DED
                default: return 2600;  // no 8+/10 for Angel — reuse 5/10
            } break;
        }
        case factionBloodRaider: {
            switch (stage) {
                case 1: return 2310;   // Blood 3/10 DED
                case 2: return 2610;   // Blood 5/10 DED
                case 3: return 2710;   // Blood Raider Temple Complex
                default: return 2710;
            } break;
        }
        case factionGuristas: {
            switch (stage) {
                case 1: return 2320;   // Guristas 3/10 DED
                case 2: return 2620;   // Guristas 5/10 DED
                case 3: return 2740;   // Pith's Penal Complex
                default: return 2940;  // The Maze (10/10)
            } break;
        }
        case factionSanshas: {
            switch (stage) {
                case 1: return 2330;   // Sansha 3/10 DED
                case 2: return 2630;   // Sansha 5/10 DED
                case 3: return 2730;   // Sansha Prison Camp
                default: return 2930;  // Centus Assembly (10/10)
            } break;
        }
        case factionSerpentis: {
            switch (stage) {
                case 1: return 2340;   // Serpentis 3/10 DED
                case 2: return 2640;   // Serpentis 5/10 DED
                case 3: return 2720;   // Serpentis Pharmalogical Plant
                default: return 2920;  // Serpentis Research Complex
            } break;
        }
        case factionRogueDrones: {
            switch (stage) {
                case 1: return 2350;   // Rogue Drone 3/10 DED
                default: return 2350;  // only 3/10 available — reuse
            } break;
        }
    }
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

    // Pick the faction's DED dungeon for this stage.
    uint32 dungeonID = ExpeditionDungeon(exp.factionID, exp.stage);
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
    sig.sigName         = ExpeditionName(exp.factionID, exp.stage);
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
