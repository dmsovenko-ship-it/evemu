#include "eve-server.h"
#include "EVEServerConfig.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "npc/NPC.h"
#include "npc/ConvoyAI.h"
#include "npc/NPCAIMgr.h"
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
    for (auto& [sysID, group] : m_systemCivs) {
        for (NPC* npc : group->members) {
            if (npc != nullptr && !npc->IsDead()) {
                npc->SystemMgr()->RemoveNPC(npc);
                npc->Delete();
            }
        }
        SafeDelete(group);
    }
    m_systemCivs.clear();
}

void CivilianMgr::Initialize() {
    m_initalized = true;
    m_processTimer = new Timer(60000);
    m_processTimer->Start(60000);
    sLog.Blue(" Civilian Manager", "Civilian Manager initialized.");
}

void CivilianMgr::Process() {
    if (!m_initalized) return;
    if (m_processTimer == nullptr || !m_processTimer->Check()) return;

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
    std::vector<SystemEntity*> routePoints;
    auto entities = sysMgr->GetEntities();
    for (auto& [id, ent] : entities) {
        if (ent == nullptr) continue;
        InventoryItemRef eRef = ent->GetSelf();
        if (eRef.get() == nullptr) continue;
        uint16 gID = eRef->groupID();
        if (ent->IsStationSE() || gID == EVEDB::invGroups::Stargate)
            routePoints.push_back(ent);
    }
    if (routePoints.size() < 2) return;

    uint32 ptA = routePoints[MakeRandomInt(0, routePoints.size() - 1)]->GetID();
    uint32 ptB = ptA;
    while (ptB == ptA)
        ptB = routePoints[MakeRandomInt(0, routePoints.size() - 1)]->GetID();

    // Industrial ship typeIDs for civilian traffic
    uint16 industrialTypes[] = {582, 583, 584, 585, 586, 587};
    uint16 typeID = industrialTypes[MakeRandomInt(0, 5)];

    uint8 count = MakeRandomInt(2, 4);
    bool sameCorp = MakeRandomFloat() > 0.5f;
    ConvoyGroup* group = new ConvoyGroup(ptA, ptB, sameCorp);

    for (uint8 i = 0; i < count; ++i) {
        GPoint pos = routePoints[0]->GetPosition();
        pos.MakeRandomPointOnSphere(5000.0);

        uint32 corpID = sDataMgr.GetFactionCorp(factionMinmatar);
        FactionData data;
        data.allianceID = 0;
        data.corporationID = corpID;
        data.factionID = factionMinmatar;
        data.ownerID = corpID;

        ItemData idata(typeID, corpID, sysID, flagNone, "Civilian", pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
        if (iRef.get() == nullptr) continue;

        NPC* npc = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, data, nullptr);
        if (npc == nullptr || !npc->Load()) {
            SafeDelete(npc);
            continue;
        }

        sysMgr->AddNPC(npc);
        npc->DestinyMgr()->SetPosition(pos);

        new ConvoyAI(npc, group, i);
        group->members.push_back(npc);

        NPCAIMgr* npcAI = npc->GetAIMgr();
        if (npcAI != nullptr)
            npcAI->Disable();
    }

    m_systemCivs[sysID] = group;
    sLog.Warning("CivilianMgr", "Spawned civilian convoy in system %u (%u ships)", sysID, count);
}

void CivilianMgr::RemoveSystemCivilians(uint32 sysID) {
    auto it = m_systemCivs.find(sysID);
    if (it == m_systemCivs.end()) return;

    ConvoyGroup* group = it->second;
    for (NPC* npc : group->members) {
        if (npc != nullptr && !npc->IsDead()) {
            npc->SystemMgr()->RemoveNPC(npc);
            npc->Delete();
        }
    }
    SafeDelete(group);
    m_systemCivs.erase(it);
    sLog.Warning("CivilianMgr", "Removed civilian convoy from system %u", sysID);
}
