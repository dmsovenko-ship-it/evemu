#include "eve-server.h"
#include "EVEServerConfig.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "npc/NPC.h"
#include "npc/ConvoyAI.h"
#include "npc/NPCAI.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/cosmicMgrs/CivilianMgr.h"
#include "ship/Ship.h"

CivilianMgr::CivilianMgr()
: m_db(nullptr), m_processTimer(nullptr), m_initalized(false)
{
}

CivilianMgr::~CivilianMgr() {
    SafeDelete(m_processTimer);
    for (auto& [sysID, group] : m_systemCivs)
        RemoveConvoy(group);
    for (auto* group : m_transitConvoys)
        RemoveConvoy(group);
    m_systemCivs.clear();
    m_transitConvoys.clear();
}

void CivilianMgr::Initialize() {
    m_initalized = true;
    m_processTimer = new Timer(60000);
    m_processTimer->Start(60000);
    sLog.Blue(" Civilian Manager", "Civilian Manager initialized.");
}

uint32 CivilianMgr::GetFactionForSystem(uint32 systemID) {
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT factionID FROM mapSolarSystems WHERE solarSystemID = %u", systemID))
    {
        DBResultRow row;
        if (res.GetRow(row) && !row.IsNull(0))
            return row.GetUInt(0);
    }
    return factionMinmatar;
}

std::vector<CivilianMgr::StargateLink> CivilianMgr::GetStargateLinks(uint32 systemID) {
    std::vector<StargateLink> links;
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT mj.stargateID, mj.celestialID, md.solarSystemID"
        " FROM mapJumps mj"
        "  JOIN mapDenormalize md ON mj.celestialID = md.itemID"
        " WHERE mj.stargateID IN ("
        "   SELECT itemID FROM mapDenormalize"
        "    WHERE solarSystemID = %u AND groupID = 10)", systemID))
    {
        return links;
    }
    DBResultRow row;
    while (res.GetRow(row))
        links.push_back({row.GetUInt(0), row.GetUInt(1), row.GetUInt(2)});
    return links;
}

void CivilianMgr::Process() {
    if (!m_initalized) return;
    if (m_processTimer == nullptr || !m_processTimer->Check()) return;

    // Process in-transit convoys (cross-system transfers)
    auto transitIt = m_transitConvoys.begin();
    while (transitIt != m_transitConvoys.end()) {
        ConvoyGroup* group = *transitIt;
        if (group->transitTimer != nullptr && group->transitTimer->Check()) {
            // Transit timer done — spawn in destination system
            ResumeCrossSystem(group);
            transitIt = m_transitConvoys.erase(transitIt);
        } else {
            ++transitIt;
        }
    }

    // Spawn/despawn civilians in loaded systems
    const auto& systems = sEntityList.GetSystems();
    for (auto& [sysID, sysMgr] : systems) {
        if (sysMgr == nullptr) continue;
        if (sysMgr->PlayerCount() == 0) {
            RemoveSystemCivilians(sysID);
            continue;
        }
        if (m_systemCivs.find(sysID) != m_systemCivs.end())
            continue;
        SpawnSystemCivilians(sysMgr);
    }
}

void CivilianMgr::SpawnSystemCivilians(SystemManager* sysMgr) {
    uint32 sysID = sysMgr->GetID();

    // Find stations and stargates for route endpoints
    std::vector<SystemEntity*> stations;
    std::vector<SystemEntity*> gates;
    auto entities = sysMgr->GetEntities();
    for (auto& [id, ent] : entities) {
        if (ent == nullptr) continue;
        InventoryItemRef eRef = ent->GetSelf();
        if (eRef.get() == nullptr) continue;
        if (ent->IsStationSE())
            stations.push_back(ent);
        else if (eRef->groupID() == EVEDB::invGroups::Stargate)
            gates.push_back(ent);
    }

    // Need at least one station and one gate for route variety
    if (stations.empty()) return;

    uint32 factionID = GetFactionForSystem(sysID);
    if (factionID == 0) factionID = factionMinmatar;

    // Pick faction-appropriate industrial ship types
    uint16 typeID;
    switch (factionID) {
        case 500001: { static uint16 c[] = {582,583,584}; typeID = c[MakeRandomInt(0,2)]; break; } // Caldari
        case 500002: { static uint16 m[] = {585,586,587}; typeID = m[MakeRandomInt(0,2)]; break; } // Minmatar
        case 500003: { static uint16 a[] = {589,590,591}; typeID = a[MakeRandomInt(0,2)]; break; } // Amarr
        case 500004: { static uint16 g[] = {592,593,594}; typeID = g[MakeRandomInt(0,2)]; break; } // Gallente
        default:     { static uint16 d[] = {582,583,584,585,586,587}; typeID = d[MakeRandomInt(0,5)]; }
    }

    uint8 count = MakeRandomInt(2, 4);
    bool sameCorp = MakeRandomFloat() > 0.5f;

    // Prefer cross-system routes if stargate connections exist
    std::vector<StargateLink> links = GetStargateLinks(sysID);
    ConvoyGroup* group = nullptr;

    // Try cross-system route: only if destination is already loaded
    if (!links.empty() && !gates.empty() && MakeRandomFloat() > 0.3f) {
        // Find a link whose destination system is loaded (has players)
        StargateLink* validLink = nullptr;
        const auto& loadedSystems = sEntityList.GetSystems();
        for (auto& link : links) {
            if (loadedSystems.find(link.destSystemID) != loadedSystems.end()) {
                validLink = &link;
                break;
            }
        }
        if (validLink != nullptr) {
            group = new ConvoyGroup(
                stations[MakeRandomInt(0, stations.size() - 1)]->GetID(),
                validLink->sourceGateID, sameCorp);
            group->destSystemID = validLink->destSystemID;
            group->sourceGateID = validLink->sourceGateID;
            group->destGateID = validLink->destGateID;
            group->factionID = factionID;
            sLog.Warning("CivilianMgr", "Cross-system route in sys%u: station→gate→sys%u",
                         sysID, validLink->destSystemID);
        }
    }
    if (group == nullptr && stations.size() >= 2) {
        // Same-system route: station → station
        uint32 idxA = MakeRandomInt(0, stations.size() - 1);
        uint32 idxB = (idxA + 1 + MakeRandomInt(0, stations.size() - 2)) % stations.size();
        group = new ConvoyGroup(stations[idxA]->GetID(), stations[idxB]->GetID(), sameCorp);
        group->factionID = factionID;
    } else if (!gates.empty()) {
        // Station → gate route
        group = new ConvoyGroup(
            stations[0]->GetID(),
            gates[MakeRandomInt(0, gates.size() - 1)]->GetID(), sameCorp);
        group->factionID = factionID;
    } else {
        return;
    }

    // Spawn ships
    for (uint8 i = 0; i < count; ++i) {
        GPoint pos = stations[0]->GetPosition();
        pos.MakeRandomPointOnSphere(5000.0);

        uint32 corpID = sDataMgr.GetFactionCorp(factionID);
        FactionData data;
        data.allianceID = 0;
        data.corporationID = corpID;
        data.factionID = factionID;
        data.ownerID = corpID;

        ItemData idata(typeID, corpID, sysID, flagNone, "Civilian", pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
        if (iRef.get() == nullptr) continue;

        NPC* npc = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, data, nullptr);
        if (npc == nullptr || !npc->Load()) {
            SafeDelete(npc);
            continue;
        }

        npc->SetIsCivilian(true);
        sysMgr->AddNPC(npc);
        npc->DestinyMgr()->SetPosition(pos);

        new ConvoyAI(npc, group, i);
        group->members.push_back(npc);
    }

    m_systemCivs[sysID] = group;
    sLog.Warning("CivilianMgr", "Spawned civilian convoy in system %u (%u ships)%s",
                 sysID, count, group->IsCrossSystem() ? " [cross-system]" : "");
}

void CivilianMgr::RemoveSystemCivilians(uint32 sysID) {
    auto it = m_systemCivs.find(sysID);
    if (it == m_systemCivs.end()) return;

    RemoveConvoy(it->second);
    m_systemCivs.erase(it);
    sLog.Warning("CivilianMgr", "Removed civilian convoy from system %u", sysID);
}

void CivilianMgr::RemoveConvoy(ConvoyGroup* group) {
    if (group == nullptr) return;
    for (NPC* npc : group->members) {
        if (npc != nullptr && !npc->IsDead()) {
            npc->SystemMgr()->RemoveNPC(npc);
            npc->Delete();
        }
    }
    SafeDelete(group);
}

void CivilianMgr::TransferCrossSystem(ConvoyGroup* group) {
    if (group == nullptr || !group->IsCrossSystem()) return;

    // Remove NPCs from current system
    for (NPC* npc : group->members) {
        if (npc != nullptr && !npc->IsDead()) {
            npc->SystemMgr()->RemoveNPC(npc);
            // Mark as in-transit (still alive, just not in any system)
        }
    }

    // Add to transit list for delayed arrival
    m_transitConvoys.push_back(group);
    sLog.Warning("CivilianMgr", "Convoy in transit to system %u (%u ships)",
                 group->destSystemID, group->members.size());
}

void CivilianMgr::ResumeCrossSystem(ConvoyGroup* group) {
    if (group == nullptr || !group->IsCrossSystem()) return;

    // Only transfer if destination is already loaded (has players)
    const auto& systems = sEntityList.GetSystems();
    auto it = systems.find(group->destSystemID);
    if (it == systems.end() || it->second == nullptr) {
        sLog.Error("CivilianMgr", "Cannot resume convoy — destination system %u not loaded",
                   group->destSystemID);
        RemoveConvoy(group);
        return;
    }
    SystemManager* destSys = it->second;

    // Find the destination stargate position
    GPoint spawnPos(0, 0, 0);
    auto entities = destSys->GetEntities();
    for (auto& [id, ent] : entities) {
        if (ent != nullptr && ent->GetID() == group->destGateID) {
            spawnPos = ent->GetPosition();
            spawnPos.MakeRandomPointOnSphere(5000.0);
            break;
        }
    }
    if (spawnPos.isZero())
        spawnPos = GPoint(MakeRandomInt(-50000, 50000), 0, MakeRandomInt(-50000, 50000));

    // Re-add NPCs to destination system
    for (NPC* npc : group->members) {
        if (npc == nullptr || npc->IsDead()) continue;

        npc->DestinyMgr()->SetPosition(spawnPos);
        destSys->AddNPC(npc);

        // Spawn at random offset from gate
        GPoint offset = spawnPos;
        offset.MakeRandomPointOnSphere(3000.0);
        npc->DestinyMgr()->SetPosition(offset);
    }

    // Track in destination system's civilian list
    m_systemCivs[group->destSystemID] = group;
    sLog.Warning("CivilianMgr", "Convoy arrived in system %u (%u ships)",
                 group->destSystemID, group->members.size());
}
