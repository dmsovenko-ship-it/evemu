#include "missions/EncounterServer.h"
#include "missions/MissionDataMgr.h"
#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/DestinyManager.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "inventory/ItemFactory.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "EVE_Corp.h"
#include "EVE_Agent.h"
#include <algorithm>
#include <string>

// Pirate NPC typeIDs for encounter missions
static const std::vector<uint32> s_guristasNPCs = { 33001, 33002, 33003, 33004 };
static const std::vector<uint32> s_angelNPCs    = { 33020, 33021, 33022, 33023 };
static const std::vector<uint32> s_serpentisNPCs= { 33040, 33041, 33042, 33043 };
static const std::vector<uint32> s_bloodRaiderNPCs = { 33060, 33061, 33062, 33063 };
static const std::vector<uint32> s_sanshaNPCs   = { 33080, 33081, 33082, 33083 };
static const std::vector<uint32> s_rogueDroneNPCs = { 33100, 33101, 33102, 33103 };

EncounterSpawnServer* EncounterSpawnServer::s_instance = nullptr;

EncounterSpawnServer::EncounterSpawnServer()
: Service("encounterSpawnServer"),
  m_nextEncounterID(1)
{
    s_instance = this;
    this->Add("GetMyEncounters", &EncounterSpawnServer::GetMyEncounters);
    this->Add("RequestActivateEncounters", &EncounterSpawnServer::RequestActivateEncounters);
    this->Add("RequestDeactivateEncounters", &EncounterSpawnServer::RequestDeactivateEncounters);
}

EncounterSpawnServer::~EncounterSpawnServer()
{
    if (s_instance == this)
        s_instance = nullptr;
}

EncounterSpawnServer* EncounterSpawnServer::Get()
{
    return s_instance;
}

// forward decls for faction target helpers defined below
static const std::vector<uint32>& GetNPCsForFaction(uint32 factionID);
static uint32 GetCorpIDForFaction(uint32 factionID);

void EncounterSpawnServer::AddEncounter(uint32 charID, uint32 missionID, uint32 agentID, uint8 agentTypeID, const std::string& name)
{
    MissionEncounter enc;
    enc.encounterID = m_nextEncounterID++;
    enc.missionID = missionID;
    enc.agentID = agentID;
    enc.charID = charID;
    enc.agentTypeID = agentTypeID;
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

// Spawn a hostile NPC cluster for an accepted Encounter mission offer and track
// it against the offer so the agent can mark the mission complete once every
// spawned target has been destroyed (Client::IsMissionComplete → IsOfferComplete).
// Returns the number of NPCs actually spawned (0 if nothing could be done).
uint32 EncounterSpawnServer::SpawnEncounterForOffer(MissionOffer& offer, uint32 destinationSystemID, const GPoint& atPos)
{
    if (offer.offerID == 0)
        return 0;
    SystemManager* pSystem = sEntityList.FindOrBootSystem(destinationSystemID);
    if (pSystem == nullptr)
        return 0;

    // Faction of the enemies — from the mission/agent corp's rat faction.
    uint32 factionID = 500014;   // default Guristas
    uint32 agentCorp = offer.originOwnerID;
    if (agentCorp != 0) {
        uint32 f = sDataMgr.GetCorpFaction(agentCorp);
        if (f != 0)
            factionID = f;
    }
    const std::vector<uint32>& npcTypes = GetNPCsForFaction(factionID);
    uint32 corpID = GetCorpIDForFaction(factionID);
    if (npcTypes.empty() || corpID == 0)
        return 0;

    FactionData faction;
        faction.allianceID = factionID;
        faction.corporationID = corpID;
        faction.factionID = factionID;
        faction.ownerID = corpID;

    // Register a tracked encounter (server-side; no client activation needed).
    MissionEncounter enc;
        enc.encounterID = m_nextEncounterID++;
        enc.missionID = offer.missionID;
        enc.agentID = offer.agentID;
        enc.charID = offer.characterID;
        enc.offerID = offer.offerID;
        enc.systemID = destinationSystemID;
        enc.agentTypeID = offer.typeID;
        enc.encounterName = offer.name;
        enc.active = true;

    uint32 spawned = 0;
    GPoint base = atPos;
    if (base.x == 0.0 && base.y == 0.0 && base.z == 0.0)
        base = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
    enc.targetPos = base;
    // 4-6 NPCs in a cluster around the site.
    uint32 npcCount = 4 + (enc.encounterID % 3);
    for (uint32 n = 0; n < npcCount; ++n) {
        uint32 typeID = npcTypes[n % npcTypes.size()];
        char buf[64];
        snprintf(buf, sizeof(buf), "Encounter NPC %u-%u", enc.encounterID, n);
        GPoint pos = base;
        pos.x += 8000 + (n % 3) * 1500;
        pos.z += (n / 3) * 1500;

        ItemData itemData(typeID, corpID, destinationSystemID, flagNone, buf, pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
        if (iRef.get() == nullptr) continue;

        // Tag this NPC as a mission target: mission:<offerID>
        iRef->SetCustomInfo(("mission:" + std::to_string(offer.offerID)).c_str());
        iRef->SaveItem();

        NPC* npc = new NPC(iRef, pSystem->GetServiceMgr(), pSystem, faction);
        if (npc == nullptr) continue;
        if (npc->Load()) {
            npc->DestinyMgr()->SetPosition(pos);
            pSystem->AddNPC(npc);
            enc.spawnedEntities.push_back(npc->GetID());
            ++spawned;
            _log(AGENT__MESSAGE, "EncounterSpawnServer: spawned mission target %s (typeID=%u) for offer %u.",
                 buf, typeID, offer.offerID);
        } else {
            SafeDelete(npc);
        }
    }

    if (spawned > 0) {
        m_encounters.emplace(enc.encounterID, enc);
        // Record the target count on the offer so IsMissionComplete can compare.
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE agtOffers SET missionNPCs = %u, missionNPCsKilled = 0, dungeonSolarSystemID = %u"
            " WHERE offerID = %u", spawned, destinationSystemID, offer.offerID);
        _log(AGENT__MESSAGE, "EncounterSpawnServer: offer %u now tracks %u mission targets in system %u.",
             offer.offerID, spawned, destinationSystemID);
    }
    return spawned;
}

void EncounterSpawnServer::OnMissionTargetKilled(uint32 entityID)
{
    for (auto& pair : m_encounters) {
        MissionEncounter& enc = pair.second;
        auto it = std::find(enc.spawnedEntities.begin(), enc.spawnedEntities.end(), entityID);
        if (it == enc.spawnedEntities.end())
            continue;
        enc.spawnedEntities.erase(it);
        _log(AGENT__MESSAGE, "EncounterSpawnServer: mission target %u destroyed (offer %u, %zu remain).",
             entityID, enc.offerID, enc.spawnedEntities.size());
        if (enc.spawnedEntities.empty())
            MarkOfferCleared(enc);
        return;
    }
}

bool EncounterSpawnServer::IsOfferComplete(uint32 offerID) const
{
    for (const auto& pair : m_encounters)
        if (pair.second.offerID == offerID)
            return pair.second.spawnedEntities.empty();
    return false;   // unknown offer — not complete
}

void EncounterSpawnServer::RegisterMiningSite(uint32 offerID, const GPoint& pos)
{
    if (offerID != 0)
        m_miningSites[offerID] = pos;
}

bool EncounterSpawnServer::GetTargetPosition(uint32 offerID, GPoint& outPos) const
{
    for (const auto& pair : m_encounters) {
        if (pair.second.offerID == offerID) {
            outPos = pair.second.targetPos;
            return true;
        }
    }
    auto mit = m_miningSites.find(offerID);
    if (mit != m_miningSites.end()) {
        outPos = mit->second;
        return true;
    }
    return false;
}

void EncounterSpawnServer::MarkOfferCleared(MissionEncounter& enc)
{
    if (enc.offerID == 0)
        return;
    DBQueryResult res;
    DBResultRow row;
    if (sDatabase.RunQuery(res, "SELECT missionNPCs FROM agtOffers WHERE offerID = %u", enc.offerID)
        && res.GetRow(row)) {
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE agtOffers SET missionNPCsKilled = missionNPCs WHERE offerID = %u", enc.offerID);
    }
    _log(AGENT__MESSAGE, "EncounterSpawnServer: offer %u cleared (all mission targets dead).", enc.offerID);
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
    GPoint playerPos = pClient->GetShipSE()->GetPosition();

    for (size_t i = 0; i < encounterList->size(); ++i) {
        PyRep* item = encounterList->GetItem(i);
        if (!item->IsInt()) continue;

        uint32 encID = item->AsInt()->value();
        auto it = m_encounters.find(encID);
        if (it == m_encounters.end() || it->second.active)
            continue;

        MissionEncounter& enc = it->second;
        enc.active = true;
        enc.systemID = pSystem->GetID();

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
        // FW mission: add visible anomaly in system
        if (enc.agentTypeID == Agents::Type::FacWar) {
            AnomalyMgr* anomMgr = pSystem->GetAnomMgr();
            if (anomMgr != nullptr) {
                enc.anomSigID = sEntityList.GetAnomalyID();
                std::string anomName = enc.encounterName + " (FW)";
                anomMgr->AddFWAnomaly(enc.anomSigID, spawnPos, anomName, factionID);
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

    uint32 systemID = it->second.systemID;
    SystemManager* pSystem = sEntityList.FindOrBootSystem(systemID);
    if (pSystem == nullptr) return;

    for (uint32 entityID : it->second.spawnedEntities) {
        NPC* pNPC = pSystem->GetNPCSE(entityID);
        if (pNPC != nullptr) {
            pSystem->RemoveNPC(pNPC);
            _log(AGENT__MESSAGE, "Despawned encounter NPC %u", entityID);
        } else {
            // Try generic entity removal
            SystemEntity* pSE = pSystem->GetSE(entityID);
            if (pSE != nullptr) {
                pSystem->RemoveEntity(pSE);
                _log(AGENT__MESSAGE, "Despawned encounter entity %u", entityID);
            }
        }
    }
    // FW mission: remove anomaly
    if (!it->second.anomSigID.empty()) {
        SystemManager* pSys = sEntityList.FindOrBootSystem(systemID);
        if (pSys != nullptr) {
            AnomalyMgr* anomMgr = pSys->GetAnomMgr();
            if (anomMgr != nullptr)
                anomMgr->RemoveFWAnomaly(it->second.anomSigID);
        }
    }

    it->second.spawnedEntities.clear();
    it->second.active = false;
}
