
/**
 * @name CivilianMgr.cpp
 *     Civilian (non-combatant NPC) management system for EVEmu
 *
 * @Author:        Allan
 * @date:          12 Feb 2017
 *
 */


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

#include <set>

/*  this class will be in charge of all non-combatant npcs ingame, hereinafter refered to as NC.
 * the purpose here is to simulate civilian actions by having ships travel from station to station.
 * this will also include jumping systems
 *
 *  mostly, it's simple AI to run an NC from point A to point B to simulate normal system traffic.
 *  Process() will check for player activity before creating/moving NCs.
 * NCs jumping into empty system will be deleted. (no reason to simulate traffic in empty system)
 * any NCs in system are 'paused' if last player jumps out.  processing will continue if a player enters system.
 * system creation will create NCs, configure their routing and begin processing
 * system unloading will delete any NCs in that system and remove their routing
 *
 *  as the system matures, it may be possible for 'shadier' npcs to travel to unknown areas (to do their deeds in secret locations)
 *    (the astute capsuleer will notice the ship NOT traveling to a planet, gate, or station, and will then know the general area to scan)
 *
 *  this will be a singleton class, in order for ships to correctly span multiple systems.
 */


CivilianMgr::CivilianMgr()
{
    m_initalized = false;
    m_processTimer = nullptr;
}

void CivilianMgr::Initialize() {
    m_initalized = true;
    m_processTimer = new Timer(60000);
    m_processTimer->Start(60000);
    sLog.Blue(" Civilian Manager", "Civilian Manager Initialized.");
}

void CivilianMgr::Process() {
    if (!m_initalized) return;
    if (!m_processTimer->Check()) return;

    // Get all systems with active players
    std::map<uint32, SystemManager*> systems;
    sEntityList.GetSystems(systems);

    for (auto& [sysID, sysMgr] : systems) {
        if (sysMgr == nullptr) continue;
        if (sysMgr->PlayerCount() == 0) {
            RemoveSystemCivilians(sysID);
            continue;
        }
        // Check if we already have civilians in this system
        if (m_systemCivs.find(sysID) != m_systemCivs.end())
            continue;

        SpawnSystemCivilians(sysMgr);
    }
}

void CivilianMgr::SpawnSystemCivilians(SystemManager* sysMgr) {
    uint32 sysID = sysMgr->GetID();

    // Pick two random stations or gates as route endpoints
    std::vector<SystemEntity*> routePoints;
    std::map<uint32, SystemEntity*> entities = sysMgr->GetEntities();
    for (auto& [id, ent] : entities) {
        if (ent != nullptr && (ent->IsStationSE() || ent->IsStargateSE()))
            routePoints.push_back(ent);
    }
    if (routePoints.size() < 2) return;

    uint32 ptA = routePoints[MakeRandomInt(0, routePoints.size() - 1)]->GetID();
    uint32 ptB = ptA;
    while (ptB == ptA)
        ptB = routePoints[MakeRandomInt(0, routePoints.size() - 1)]->GetID();

    // Pick a civilian freighter typeID
    uint16 freighterTypes[] = {582, 583, 584, 585, 586, 587, 589, 590};
    uint16 typeID = freighterTypes[MakeRandomInt(0, 7)];

    // Create 3-5 civilian NPCs as a convoy
    uint8 count = MakeRandomInt(3, 5);
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

        ConvoyAI* ai = new ConvoyAI(npc, group, i);
        group->members.push_back(npc);

        // Disable standard NPC AI for civilians
        NPCAI* npcAI = npc->GetAIMgr();
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

