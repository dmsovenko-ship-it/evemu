#include "missions/EncounterServer.h"
#include "missions/MissionDataMgr.h"
#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/DestinyManager.h"
#include "inventory/ItemFactory.h"
#include "StaticDataMgr.h"
#include "EVE_Corp.h"

// Pirate NPC typeIDs for encounter missions
static const std::vector<uint32> s_guristasNPCs = { 5832, 5833, 5834, 5835 };
static const std::vector<uint32> s_angelNPCs    = { 5861, 5862, 5863, 5864 };
static const std::vector<uint32> s_serpentisNPCs= { 5878, 5879, 5880, 5881 };
static const std::vector<uint32> s_bloodRaiderNPCs = { 5891, 5892, 5893, 5894 };
static const std::vector<uint32> s_sanshaNPCs   = { 5905, 5906, 5907, 5908 };
static const std::vector<uint32> s_rogueDroneNPCs = { 5911, 5912, 5913, 5914 };

EncounterSpawnServer::EncounterSpawnServer()
: Service("encounterSpawnServer"),
  m_nextEncounterID(1)
{
    this->Add("GetMyEncounters", &EncounterSpawnServer::GetMyEncounters);
    this->Add("RequestActivateEncounters", &EncounterSpawnServer::RequestActivateEncounters);
    this->Add("RequestDeactivateEncounters", &EncounterSpawnServer::RequestDeactivateEncounters);
}

EncounterSpawnServer::~EncounterSpawnServer()
{
}

void EncounterSpawnServer::AddEncounter(uint32 charID, uint32 missionID, uint32 agentID, const std::string& name)
{
    MissionEncounter enc;
    enc.encounterID = m_nextEncounterID++;
    enc.missionID = missionID;
    enc.agentID = agentID;
    enc.charID = charID;
    enc.encounterName = name;
    enc.active = false;
    m_encounters.emplace(enc.encounterID, enc);
}

void EncounterSpawnServer::RemoveEncounter(uint32 encounterID)
{
    auto it = m_encounters.find(encounterID);
    if (it != m_encounters.end())
        m_encounters.erase(it);
}

PyResult EncounterSpawnServer::GetMyEncounters(PyCallArgs& call)
{
    _log(AGENT__INFO, "EncounterSpawnServer::GetMyEncounters() - size=%lli", call.tuple->size());

    uint32 charID = call.client->GetCharID();
    PyList* result = new PyList();

    for (auto& pair : m_encounters) {
        MissionEncounter& enc = pair.second;
        if (enc.charID == charID) {
            PyDict* entry = new PyDict();
            entry->SetItem(new PyString("encounterID"), new PyInt(enc.encounterID));
            entry->SetItem(new PyString("encounterName"), new PyString(enc.encounterName));
            entry->SetItem(new PyString("agentID"), new PyInt(enc.agentID));
            entry->SetItem(new PyString("active"), enc.active ? PyStatic.NewTrue() : PyStatic.NewFalse());
            result->AddItem(entry);
        }
    }
    return result;
}

static const std::vector<uint32>& GetNPCsForFaction(uint32 factionID)
{
    switch (factionID) {
        case 500011: return s_angelNPCs;       // Angel Cartel
        case 500012: return s_bloodRaiderNPCs;  // Blood Raiders
        case 500013: return s_serpentisNPCs;    // Serpentis
        case 500014: return s_guristasNPCs;     // Guristas
        case 500019: return s_sanshaNPCs;       // Sansha's Nation
        case 500020: return s_rogueDroneNPCs;   // Rogue Drones / SoE
        default:     return s_guristasNPCs;     // fallback
    }
}

static uint32 GetCorpIDForFaction(uint32 factionID)
{
    switch (factionID) {
        case 500011: return 1000157;  // Angel Cartel
        case 500012: return 1000064;  // Blood Raiders
        case 500013: return 1000165;  // Serpentis
        case 500014: return 1000166;  // Guristas
        case 500019: return 1000167;  // Sansha
        case 500020: return 1000179;  // Sisters of EVE
        default:     return 1000166;  // fallback
    }
}

PyResult EncounterSpawnServer::RequestActivateEncounters(PyCallArgs& call, PyList* encounterList)
{
    _log(AGENT__INFO, "EncounterSpawnServer::RequestActivateEncounters() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    PyList* result = new PyList();
    PyDict* logResult = new PyDict();
    logResult->SetItem(new PyString("log"), PyStatic.NewNone());

    Client* pClient = call.client;
    SystemManager* pSystem = pClient->SystemMgr();
    GPoint playerPos = pClient->GetPosition();

    for (size_t i = 0; i < encounterList->size(); ++i) {
        PyRep* item = encounterList->GetItem(i);
        if (!item->IsInt()) continue;

        uint32 encID = item->AsInt()->value();
        auto it = m_encounters.find(encID);
        if (it == m_encounters.end() || it->second.active)
            continue;

        MissionEncounter& enc = it->second;
        enc.active = true;

        // Determine faction from mission (default Guristas)
        uint32 factionID = 500014;
        const std::vector<uint32>& npcTypes = GetNPCsForFaction(factionID);
        uint32 corpID = GetCorpIDForFaction(factionID);

        FactionData faction;
        faction.allianceID = factionID;
        faction.corporationID = corpID;
        faction.factionID = factionID;
        faction.ownerID = corpID;

        // Spawn 4-6 NPCs in a cluster near the player
        uint32 npcCount = 4 + (enc.encounterID % 3); // 4-6 NPCs
        GPoint spawnPos = playerPos;
        spawnPos.x += 8000; // spawn in front of player's position

        for (uint32 n = 0; n < npcCount; ++n) {
            uint32 typeID = npcTypes[n % npcTypes.size()];
            char buf[64];
            snprintf(buf, sizeof(buf), "Encounter NPC %u-%u", encID, n);
            GPoint pos = spawnPos;
            pos.x += (n % 3) * 1500;
            pos.z += (n / 3) * 1500;

            ItemData itemData(typeID, corpID, pSystem->GetID(), flagNone, buf, pos);
            InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
            if (iRef.get() == nullptr) continue;

            NPC* npc = new NPC(iRef, pSystem->GetServiceMgr(), pSystem, faction);
            if (npc == nullptr) continue;

            if (npc->Load()) {
                npc->DestinyMgr()->SetPosition(pos);
                pSystem->AddNPC(npc);
                enc.spawnedEntities.push_back(npc->GetID());

                _log(AGENT__MESSAGE, "Spawned encounter NPC %s (typeID=%u) at (%.0f, %.0f, %.0f)",
                     buf, typeID, pos.x, pos.y, pos.z);
            } else {
                SafeDelete(npc);
            }
        }
        result->AddItem(new PyInt(encID));
    }

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, result);
    rsp->SetItem(1, logResult);
    return rsp;
}

PyResult EncounterSpawnServer::RequestDeactivateEncounters(PyCallArgs& call, PyList* encounterList)
{
    _log(AGENT__INFO, "EncounterSpawnServer::RequestDeactivateEncounters() - size=%lli", call.tuple->size());

    PyList* result = new PyList();
    PyDict* logResult = new PyDict();
    logResult->SetItem(new PyString("log"), PyStatic.NewNone());

    for (size_t i = 0; i < encounterList->size(); ++i) {
        PyRep* item = encounterList->GetItem(i);
        if (!item->IsInt()) continue;

        uint32 encID = item->AsInt()->value();
        DespawnEncounters(encID);
        result->AddItem(new PyInt(encID));
    }

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, result);
    rsp->SetItem(1, logResult);
    return rsp;
}

void EncounterSpawnServer::DespawnEncounters(uint32 encounterID)
{
    auto it = m_encounters.find(encounterID);
    if (it == m_encounters.end()) return;

    for (uint32 entityID : it->second.spawnedEntities) {
        SystemEntity* pSE = sEntityList.FindByID(entityID);
        if (pSE != nullptr) {
            pSE->Remove();
            _log(AGENT__MESSAGE, "Despawned encounter entity %u", entityID);
        }
    }
    it->second.spawnedEntities.clear();
    it->second.active = false;
}
