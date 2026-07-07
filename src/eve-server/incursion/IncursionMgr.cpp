#include "eve-server.h"

#include "incursion/IncursionMgr.h"
#include "EVE_Incursion.h"
#include "EntityList.h"
#include "Client.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/cosmicMgrs/DungeonMgr.h"
#include "system/cosmicMgrs/SpawnMgr.h"
#include "system/Celestial.h"

IncursionMgr::IncursionMgr()
{
}

void IncursionMgr::Process()
{
    sLog.Warning("IncursionMgr", "Process() called");

    // Check if any incursions exist, auto-start one if none active
    DBQueryResult activeRes;
    bool hasActive = false;
    if (sDatabase.RunQuery(activeRes,
        "SELECT COUNT(*) FROM incursions WHERE state > 0"))
    {
        DBResultRow activeRow;
        if (activeRes.GetRow(activeRow))
            hasActive = (activeRow.GetUInt(0) > 0);
    }

    if (!hasActive) {
        sLog.Warning("IncursionMgr", "No active incursions, starting new one...");
        DBQueryResult constRes;
        if (sDatabase.RunQuery(constRes,
            "SELECT constellationID, regionID FROM mapConstellations "
            "WHERE constellationID IN (20000383, 20000267, 20000341, 20000446, 20000370, "
            "20000432, 20000034, 20000066, 20000106, 20000134, 20000208) "
            "AND constellationID NOT IN (SELECT constellationID FROM incursions WHERE state > 0) "
            "ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow constRow;
            if (constRes.GetRow(constRow)) {
                StartIncursion(factionSanshas, constRow.GetUInt(0));
            } else {
                sLog.Warning("IncursionMgr", "Could not find valid constellation for incursion");
            }
        }
        return;
    }

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT incursionID, state, lastUpdated FROM incursions WHERE state > 0"))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 incursionID = row.GetUInt(0);
        uint8 state = row.GetUInt(1);
        double lastUpdated = row.GetDouble(2);

        UpdateInfluence(incursionID);
        ProgressStateMachine(incursionID);
        SpawnSites(incursionID);
    }
}

void IncursionMgr::StartIncursion(uint32 factionID, uint32 constellationID)
{
    // Find a random system in constellation for staging
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT solarSystemID, regionID FROM mapSolarSystems "
        "WHERE constellationID = %u ORDER BY RAND() LIMIT 1",
        constellationID))
        return;

    DBResultRow row;
    if (!res.GetRow(row))
        return;

    uint32 stagingSystem = row.GetUInt(0);
    uint32 regionID = row.GetUInt(1);
    double now = GetFileTimeNow();

    // Pick rewardGroupID based on state progression
    // state 1 (established) = rewardGroup 1 (VG), state 2 (mobilized) = rewardGroup 3 (HQ)
    uint32 rewardGroupID = 1;

    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO incursions (factionID, stagingSolarSystemID, constellationID, regionID, "
        "state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate, lastUpdated) "
        "VALUES (%u, %u, %u, %u, 1, 1.0, 0, %u, 0, 30, 0.01, %.0f)",
        factionID, stagingSystem, constellationID, regionID, rewardGroupID, now);

    // Mark all systems in constellation as incursion systems
    DBQueryResult sysRes;
    sDatabase.RunQuery(sysRes,
        "SELECT solarSystemID FROM mapSolarSystems WHERE constellationID = %u",
        constellationID);

    DBResultRow sRow;
    while (sysRes.GetRow(sRow)) {
        uint32 sysID = sRow.GetUInt(0);
        uint8 sceneType = Incursion::scenesType::vanguard;
        if (sysID == stagingSystem)
            sceneType = Incursion::scenesType::staging;

        sDatabase.RunQuery(err,
            "INSERT INTO incursionSystems (incursionID, solarSystemID, sceneType, influence) "
            "VALUES ((SELECT incursionID FROM incursions WHERE stagingSolarSystemID = %u), %u, %u, 1.0)",
            stagingSystem, sysID, sceneType);
    }

    NotifyClients(0);
    sLog.Warning("IncursionMgr", "Incursion started in constellation %u (staging: %u)", constellationID, stagingSystem);
}

void IncursionMgr::EndIncursion(uint32 incursionID)
{
    double now = GetFileTimeNow();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE incursions SET state = 0, lastUpdated = %.0f WHERE incursionID = %u",
        now, incursionID);
    sDatabase.RunQuery(err,
        "DELETE FROM incursionSystems WHERE incursionID = %u", incursionID);

    m_activeSystems.clear();
    NotifyClients(incursionID);
    sLog.Warning("IncursionMgr", "Incursion %u ended", incursionID);
}

void IncursionMgr::OnSiteCompleted(uint32 incursionID, uint32 solarSystemID, uint8 sceneType)
{
    m_activeSystems.erase(solarSystemID);

    // Reduce influence based on site type and wave progress
    // VG = small hit, AS = medium, HQ = big, Staging = small
    // Mothership (sceneType=1000001) ends the incursion

    if (sceneType == Incursion::scenesType::boss) {
        EndIncursion(incursionID);
        return;
    }

    float reduction = 0.0f;
    switch (sceneType) {
        case Incursion::scenesType::vanguard:      reduction = 0.01f; break;
        case Incursion::scenesType::assault:        reduction = 0.02f; break;
        case Incursion::scenesType::headquarters:   reduction = 0.04f; break;
        case Incursion::scenesType::staging:        reduction = 0.01f; break;
        default: return;
    }

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE incursionSystems SET influence = GREATEST(0.0, influence - %f) "
        "WHERE incursionID = %u AND solarSystemID = %u",
        reduction, incursionID, solarSystemID);

    // Update overall incursion influence
    sDatabase.RunQuery(err,
        "UPDATE incursions SET influence = (SELECT AVG(influence) FROM incursionSystems "
        "WHERE incursionID = %u) WHERE incursionID = %u",
        incursionID, incursionID);

    // Check if mothership should spawn (all systems at 0 influence)
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM incursionSystems WHERE incursionID = %u AND influence > 0.0",
        incursionID))
    {
        DBResultRow row;
        if (res.GetRow(row) && row.GetUInt(0) == 0)
            sDatabase.RunQuery(err,
                "UPDATE incursions SET hasBoss = 1 WHERE incursionID = %u", incursionID);
    }

    NotifyClients(incursionID);
}

void IncursionMgr::ProgressStateMachine(uint32 incursionID)
{
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT state, lastUpdated, stagingSolarSystemID FROM incursions WHERE incursionID = %u",
        incursionID);

    DBResultRow row;
    if (!res.GetRow(row))
        return;

    uint8 state = row.GetUInt(0);
    double lastUpdated = row.GetDouble(1);
    double now = GetFileTimeNow();
    double elapsedHours = (now - lastUpdated) / EvE::Time::Hour;

    uint8 newState = state;
    switch (state) {
        case 1: // established
            if (elapsedHours >= 120.0) newState = 2; // 5 days → mobilized
            break;
        case 2: // mobilized
            if (elapsedHours >= 48.0) newState = 3; // 2 days → withdrawal
            break;
        case 3: // withdrawal
            if (elapsedHours >= 24.0) {
                EndIncursion(incursionID);
                return;
            }
            break;
    }

    if (newState != state) {
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE incursions SET state = %u, lastUpdated = %.0f WHERE incursionID = %u",
            newState, now, incursionID);
        sLog.Warning("IncursionMgr", "Incursion %u state: %u -> %u", incursionID, state, newState);
    }
}

void IncursionMgr::UpdateInfluence(uint32 incursionID)
{
    // Natural influence regen: +1% every 20 minutes in active systems
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT incursionID, COUNT(*) FROM incursionSystems "
        "WHERE influence < 1.0 AND incursionID = %u", incursionID))
    {
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE incursionSystems SET influence = LEAST(1.0, influence + 0.01) "
            "WHERE incursionID = %u AND influence < 1.0 AND MOD(MINUTE(NOW()), 20) = 0",
            incursionID);
    }
}

void IncursionMgr::SpawnSites(uint32 incursionID)
{
    sLog.Warning("IncursionMgr", "SpawnSites called for incursion %u", incursionID);

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT iss.solarSystemID, iss.sceneType, iss.influence, i.regionID "
        "FROM incursionSystems iss "
        "JOIN incursions i ON iss.incursionID = i.incursionID "
        "WHERE iss.incursionID = %u AND i.state > 0",
        incursionID))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 solarSystemID = row.GetUInt(0);
        uint8 sceneType = row.GetUInt(1);
        float influence = row.GetFloat(2);
        uint32 regionID = row.GetUInt(3);

        if (influence <= 0.0f)
            continue;

        if (m_activeSystems.find(solarSystemID) != m_activeSystems.end())
            continue;

        SystemManager* sMgr = sEntityList.FindOrBootSystem(solarSystemID);
        if (sMgr == nullptr)
            continue;

        uint32 playerCount = sMgr->PlayerCount();
        if (playerCount == 0)
            continue;

        uint8 spawnChance = 75;
        if (sceneType == Incursion::scenesType::staging)
            spawnChance = 40;
        spawnChance = static_cast<uint8>(std::min<uint32>(spawnChance + (playerCount - 1) * 10, 95));

        if (MakeRandomInt(0, 100) > spawnChance)
            continue;

        DBQueryResult bossRes;
        uint8 hasBoss = 0;
        if (sDatabase.RunQuery(bossRes,
            "SELECT hasBoss FROM incursions WHERE incursionID = %u", incursionID))
        {
            DBResultRow bossRow;
            if (bossRes.GetRow(bossRow))
                hasBoss = bossRow.GetUInt(0);
        }

        if (hasBoss == 1 && sceneType == Incursion::scenesType::headquarters) {
            SpawnMothership(incursionID, solarSystemID);
            m_activeSystems.insert(solarSystemID);
            continue;
        }

        GPoint pos;
        pos.x = MakeRandomFloat(-1.0e12, 1.0e12);
        pos.y = MakeRandomFloat(-1.0e12, 1.0e12);
        pos.z = MakeRandomFloat(-1.0e12, 1.0e12);

        uint32 ownerID = sDataMgr.GetFactionCorp(factionSanshas);

        // Create CelestialSE marker for scanner
        ItemData iData(EVEDB::invTypes::CosmicAnomaly, ownerID, solarSystemID,
                       flagNone, "Incursion Site", pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(iData);
        if (iRef.get() == nullptr)
            continue;

        iRef->SetAttribute(AttrSignatureRadius, 1000.0, false);
        iRef->SetCustomInfo("incursion_site");
        iRef->SaveItem();

        CelestialSE* cSE = new CelestialSE(iRef, sMgr->GetServiceMgr(), sMgr);
        sMgr->AddEntity(cSE, false);

        AnomalyMgr* anomMgr = sMgr->GetAnomMgr();
        if (anomMgr != nullptr)
            anomMgr->AddSignal(cSE);

        // Spawn NPCs in the same bubble
        SystemBubble* bubble = cSE->SysBubble();
        if (bubble != nullptr)
            sMgr->GetSpawnMgr()->DoSpawnForIncursion(bubble, regionID, sceneType, incursionID);

        m_activeSystems.insert(solarSystemID);

        sLog.Warning("IncursionMgr", "Spawned incursion site in system %u (sceneType=%u marker=%u)",
            solarSystemID, sceneType, iRef->itemID());
    }
}

bool IncursionMgr::IsIncursionSystem(uint32 solarSystemID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM incursionSystems iss "
        "JOIN incursions i ON iss.incursionID = i.incursionID "
        "WHERE iss.solarSystemID = %u AND i.state > 0",
        solarSystemID))
        return false;
    DBResultRow row;
    if (res.GetRow(row))
        return row.GetUInt(0) > 0;
    return false;
}

uint8 IncursionMgr::GetSceneType(uint32 solarSystemID, uint32 incursionID)
{
    DBQueryResult res;
    if (incursionID > 0) {
        sDatabase.RunQuery(res,
            "SELECT sceneType FROM incursionSystems "
            "WHERE incursionID = %u AND solarSystemID = %u",
            incursionID, solarSystemID);
    } else {
        sDatabase.RunQuery(res,
            "SELECT iss.sceneType FROM incursionSystems iss "
            "JOIN incursions i ON iss.incursionID = i.incursionID "
            "WHERE iss.solarSystemID = %u AND i.state > 0 LIMIT 1",
            solarSystemID);
    }
    DBResultRow row;
    if (res.GetRow(row))
        return row.GetUInt(0);
    return 0;
}

void IncursionMgr::SpawnMothership(uint32 incursionID, uint32 solarSystemID)
{
    SystemManager* sMgr = sEntityList.FindOrBootSystem(solarSystemID);
    if (sMgr == nullptr)
        return;

    // Create a bubble at a random position in the system
    GPoint pos;
    pos.x = MakeRandomFloat(-1.0e12, 1.0e12);
    pos.y = MakeRandomFloat(-1.0e12, 1.0e12);
    pos.z = MakeRandomFloat(-1.0e12, 1.0e12);

    // Use SpawnMgr to spawn the mothership NPCs
    SpawnMgr* spawnMgr = sMgr->GetSpawnMgr();
    if (spawnMgr == nullptr)
        return;

    // Create a temporary bubble for the mothership encounter
    SystemBubble* bubble = sBubbleMgr.FindBubble(solarSystemID, pos);
    if (bubble == nullptr)
        return;

    // Spawn mothership NPCs with boss sceneType
    spawnMgr->DoSpawnMothership(bubble, incursionID);

    sLog.Warning("IncursionMgr", "Spawned incursion mothership in system %u", solarSystemID);

    // Reset hasBoss so it doesn't re-spawn every tick
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE incursions SET hasBoss = 2 WHERE incursionID = %u", incursionID);
}

void IncursionMgr::DespawnSites(uint32 incursionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "DELETE FROM incursionSystems WHERE incursionID = %u", incursionID);
}

void IncursionMgr::NotifyClients(uint32 incursionID)
{
    // Notify all online clients about incursion state change
    // Sends an OnIncursionUpdated notification
    PyTuple* payload = new PyTuple(1);
    if (incursionID > 0)
        payload->SetItem(0, new PyInt(incursionID));
    else
        payload->SetItem(0, PyStatic.NewNone());

    std::vector<Client*> clients;
    sEntityList.GetClients(clients);
    for (auto client : clients)
        client->SendNotification("OnIncursionUpdated", "clientID", payload, false);
}
