#include "npc/FactionPatrolManager.h"

#include "eve-server.h"

#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/DestinyManager.h"
#include "inventory/ItemFactory.h"
#include "StaticDataMgr.h"

// Border constellations by faction (from FW war zones, highsec side)
// Amarr: Devoid(Semou,Jayai), BleakLands(Sasen,Tandoiras,Vaarma)
// Minmatar: Heimatar(Hed,Huvilma), Metropolis(Essin,Tiat,Eugidi,Angils,Aldodan)
// Caldari: Citadel(Ieyama,Isoma), BlackRise(Inolari,Ishaga,Kurala,Okakuola)
// Gallente: Essence(Jeon,Vieres), VergeVendor(Obray,Woenckee), Placid(Amevync,Pegeler,Serthoulde)
static const std::map<uint32, std::pair<uint32, std::vector<uint32>>> s_borderConstellations = {
    // Amarr border: Devoid + Bleak Lands constellations
    {500003, {0, {20000294, 20000295, 20000296, 20000297, 20000298, 20000299, 20000300, 20000301}}},
    // Minmatar border: Heimatar + Metropolis constellations
    {500002, {0, {20000302, 20000303, 20000304, 20000305, 20000306, 20000307, 20000308, 20000309, 20000310}}},
    // Caldari border: Citadel + Black Rise constellations
    {500001, {0, {20000016, 20000017, 20000018, 20000019, 20000020, 20000021, 20000022, 20000023}}},
    // Gallente border: Essence + Verge Vendor + Placid
    {500004, {0, {20000044, 20000045, 20000046, 20000047, 20000048, 20000049, 20000050, 20000051, 20000052, 20000053, 20000054, 20000055}}},
};

// Faction navy patrol typeIDs by faction (group 288 Faction_Drone or 182 Police_Drone)
static const std::map<uint32, std::vector<uint32>> s_patrolTypes = {
    {500001, {10046, 10047, 10048}},  // Caldari Navy
    {500002, {9997, 9998, 9999}},      // Minmatar Republic Fleet
    {500003, {9959, 9960, 9999}},      // Amarr Navy
    {500004, {10052, 10053, 10054}},   // Gallente Federation Navy
};

void FactionPatrolManager::SpawnPatrols(SystemManager* sysMgr)
{
    if (sysMgr == nullptr) return;
    float sec = sysMgr->GetSystemSecurityRating();
    if (sec < 0.5f) return;  // only highsec border systems

    uint32 systemFactionID = sysMgr->GetSystemRef()->factionID();
    if (systemFactionID == 0)
        systemFactionID = sDataMgr.GetRegionFaction(sysMgr->GetRegionID());
    if (systemFactionID == 0) return;

    // Check if this system is in a border constellation
    auto it = s_borderConstellations.find(systemFactionID);
    if (it == s_borderConstellations.end()) return;

    uint32 constellationID = sysMgr->GetConstellationID();
    bool isBorder = false;
    for (uint32 cid : it->second.second) {
        if (cid == constellationID) { isBorder = true; break; }
    }
    if (!isBorder) return;

    // Determine patrol count based on proximity to border (simplified: 1-3 per gate)
    // Deeper in highsec = fewer patrols
    float spawnChance = 0.3f;
    if (sec > 0.8f) spawnChance = 0.15f;
    else if (sec > 0.6f) spawnChance = 0.25f;

    auto pTypes = s_patrolTypes.find(systemFactionID);
    if (pTypes == s_patrolTypes.end()) return;

    auto gates = sysMgr->GetGates();
    for (auto& [gateID, pGate] : gates) {
        if (pGate == nullptr) continue;
        if (MakeRandomFloat(0.0f, 1.0f) >= spawnChance) continue;

        GPoint gatePos = pGate->GetPosition();
        uint32 count = 0;

        auto spawnPatrol = [&](uint16 typeID) {
            GPoint pos = gatePos;
            float angle = (float)MakeRandomInt(0, 6283) / 1000.0f;
            float dist = (float)MakeRandomInt(5000, 15000);
            pos.x += dist * cosf(angle);
            pos.z += dist * sinf(angle);

            FactionData faction;
            faction.allianceID = 0;
            faction.corporationID = sDataMgr.GetFactionCorp(systemFactionID);
            faction.factionID = systemFactionID;
            faction.ownerID = faction.corporationID;

            ItemData itemData(typeID, faction.ownerID, sysMgr->GetID(), flagNone, "", pos);
            InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
            if (iRef.get() == nullptr) return;

            NPC* npc = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, faction);
            if (npc == nullptr) return;
            if (!npc->Load()) { delete npc; return; }

            npc->DestinyMgr()->SetPosition(pos);
            npc->DestinyMgr()->Orbit(pGate, (uint32)MakeRandomInt(5000, 10000));
            npc->DestinyMgr()->SetSpeedFraction(0.2f, true);
            sysMgr->AddNPC(npc);
            ++count;
        };

        // Spawn 1-3 ships per gate
        uint8 numShips = 1 + MakeRandomInt(0, 2);
        for (uint8 i = 0; i < numShips; ++i) {
            uint16 typeID = pTypes->second[MakeRandomInt(0, pTypes->second.size() - 1)];
            spawnPatrol(typeID);
        }
    }
}
