#include "npc/CustomsNPCManager.h"

#include "eve-server.h"

#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/DestinyManager.h"
#include "inventory/ItemFactory.h"
#include "StaticDataMgr.h"
#include "EVE_Corp.h"

// Per-faction customs typeIDs (group 446):
// Commissioner = battleship-class, Agent = frigate-class
static const std::map<uint32, std::pair<uint16, uint16>> s_customsTypes = {
    { factionCaldari,  { 19367, 19385 } },  // Caldari Customs Commissioner + Agent
    { factionMinmatar, { 19370, 19388 } },  // Minmatar Customs Commander + Patroller
    { factionAmarr,    { 19371, 17286 } },  // Amarr Customs General + Captain
    { factionGallente, { 19369, 19383 } },  // Gallente Customs Major + Official
};

// Fallback for non-empire high-sec (CONCORD)
static const uint32 s_defaultTypeIDs[2] = { 19563, 19564 };  // CONCORD Commander + Official

void CustomsNPCManager::SpawnCustomsNPCs(SystemManager* sysMgr)
{
    if (sysMgr == nullptr) return;

    float sec = sysMgr->GetSystemSecurityRating();
    if (sec < 0.5f) return;  // only highsec

    uint32 systemFactionID = sysMgr->GetSystemRef()->factionID();
    // Many highsec systems have factionID=0 — use region faction instead
    if (systemFactionID == 0)
        systemFactionID = sDataMgr.GetRegionFaction(sysMgr->GetRegionID());

    // Determine which typeIDs to use for this faction
    uint16 commissionerTypeID = s_defaultTypeIDs[0];
    uint16 agentTypeID = s_defaultTypeIDs[1];

    auto it = s_customsTypes.find(systemFactionID);
    if (it != s_customsTypes.end()) {
        commissionerTypeID = it->second.first;
        agentTypeID = it->second.second;
    }

    // Determine corp ownership for customs NPCs
    uint32 corpID = sDataMgr.GetFactionCorp(systemFactionID);
    if (corpID == 0) {
        switch (systemFactionID) {
            case factionMinmatar: corpID = corpRepublicFleet; break;
            default:              corpID = corpCONCORD;       break;
        }
    }

    FactionData faction;
    faction.allianceID = systemFactionID;
    faction.corporationID = corpID;
    faction.factionID = systemFactionID;
    faction.ownerID = corpID;

    // Iterate over all gates
    auto gates = sysMgr->GetGates();
    for (auto& [gateID, pGate] : gates) {
        if (pGate == nullptr) continue;

        // 20% chance for customs NPCs at this gate
        if (MakeRandomFloat(0.0f, 1.0f) >= 0.20f) continue;

        GPoint gatePos = pGate->GetPosition();

        uint32 count = 0;

        auto spawnNPC = [&](uint16 typeID, const char* role, uint32 index) {
            GPoint pos = gatePos;
            float angle = (float)MakeRandomInt(0, 6283) / 1000.0f;
            float dist = (float)MakeRandomInt(5000, 10000);
            pos.x += dist * cosf(angle);
            pos.z += dist * sinf(angle);

            char name[64];
            snprintf(name, sizeof(name), "%s %s %u",
                     sDataMgr.GetFactionName(systemFactionID).c_str(), role, index);

            ItemData itemData(typeID, corpID, sysMgr->GetID(), flagNone, name, pos);
            InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
            if (iRef.get() == nullptr) return;

            NPC* npc = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, faction);
            if (npc == nullptr) return;

            if (npc->Load()) {
                npc->DestinyMgr()->SetPosition(pos);
                // Orbit the gate at 8-12km at minimum speed
                npc->DestinyMgr()->Orbit(pGate, (uint32)MakeRandomInt(8000, 12000));
                npc->DestinyMgr()->SetSpeedFraction(0.15f, true);
                sysMgr->AddNPC(npc);
                ++count;
            } else {
                delete npc;
            }
        };

        // Spawn 2 commissioners + 2 agents
        spawnNPC(commissionerTypeID, "Commissioner", 1);
        spawnNPC(commissionerTypeID, "Commissioner", 2);
        spawnNPC(agentTypeID, "Agent", 1);
        spawnNPC(agentTypeID, "Agent", 2);

        if (count > 0) {
            sysMgr->AddCustomsGate(gateID);
            sLog.Log("CustomsNPCManager", "Spawned %u customs NPCs at gate %u in %s (faction %u)",
                     count, gateID, sysMgr->GetName(), systemFactionID);
        }
    }
}
