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
#include "corporation/LPService.h"

IncursionMgr::IncursionMgr()
{
}

void IncursionMgr::Process()
{
    // Init spawn timer on first call
    if (!m_spawnTimer.Enabled())
        m_spawnTimer.Start(300000);  // 5 min

    // Allow up to 5 simultaneous incursions (1 HS, 1 LS, 3 NS) with 12-36h respawn
    DBQueryResult activeRes;
    uint32 activeCount = 0;
    bool hasHS = false, hasLS = false;
    if (sDatabase.RunQuery(activeRes,
        "SELECT i.incursionID, i.lastUpdated, "
        "  (SELECT COALESCE(AVG(s.security),0) FROM mapSolarSystems s JOIN incursionSystems isys ON s.solarSystemID=isys.solarSystemID WHERE isys.incursionID=i.incursionID) as avgSec "
        "FROM incursions i WHERE i.state > 0"))
    {
        DBResultRow aRow;
        while (activeRes.GetRow(aRow)) {
            ++activeCount;
            float avgSec = aRow.GetFloat(2);
            if (avgSec >= 0.5f) hasHS = true;
            else hasLS = true;
        }
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
        // Spawn sites only every 5 minutes to avoid constant DB queries
        if (m_spawnTimer.Check(false))
            SpawnSites(incursionID);
    }

    // Try to start new incursions if below the cap
    if (m_spawnTimer.Check(false) && activeCount < 5) {
        bool spawnHS = !hasHS;
        bool spawnLS = !hasLS && hasHS;
        bool spawnNS = !spawnHS && !spawnLS;
        double now = GetFileTimeNow();

        DBQueryResult lastRes;
        if (sDatabase.RunQuery(lastRes,
            "SELECT MAX(lastUpdated) FROM incursions WHERE state = 0 "
            "AND (SELECT AVG(s.security) FROM mapSolarSystems s "
            "  JOIN incursionSystems isys ON s.solarSystemID=isys.solarSystemID "
            "  WHERE isys.incursionID=incursions.incursionID) %s",
            spawnHS ? ">= 0.5" : spawnLS ? "BETWEEN 0.0 AND 0.49" : "< 0.0"))
        {
            DBResultRow lastRow;
            if (lastRes.GetRow(lastRow) && !lastRow.IsNull(0)) {
                double lastTime = lastRow.GetDouble(0);
                double elapsed = (now - lastTime) / EvE::Time::Hour;
                if (elapsed < 12 + MakeRandomInt(0, 24)) {
                    _log(COSMIC_MGR__TRACE, "IncursionMgr: %s cooldown active (%.1f/12-36h)",
                         spawnHS?"HS":spawnLS?"LS":"NS", elapsed);
                    return;
                }
            }
        }

        sLog.Warning("IncursionMgr", "Starting new incursion (%u active, need%s%s%s)...",
                     activeCount, spawnHS?" HS":"", spawnLS?" LS":"", spawnNS?" NS":"");
        DBQueryResult constRes;
        if (sDatabase.RunQuery(constRes,
            "SELECT constellationID, regionID FROM mapConstellations "
            "WHERE constellationID IN (20000383, 20000267, 20000341, 20000446, 20000370, "
            "20000432, 20000034, 20000066, 20000106, 20000134, 20000208) "
            "AND constellationID NOT IN (SELECT constellationID FROM incursions WHERE state > 0) "
            "ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow constRow;
            if (constRes.GetRow(constRow))
                StartIncursion(factionSanshas, constRow.GetUInt(0));
        }
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
    // Send OnTaleStart — client needs taleData for HUD activation
    // Query the new incursion ID
    uint32 newID = sDatabase.GetLastInsertID();
    if (newID > 0) {
        // Build taleData from the just-inserted incursion
        std::vector<Client*> clients;
        sEntityList.GetClients(clients);

        // Count incursion systems
        DBQueryResult cntRes;
        uint32 sysCount = 0;
        if (sDatabase.RunQuery(cntRes, "SELECT COUNT(*) FROM incursionSystems WHERE incursionID = %u", newID)) {
            DBResultRow cntRow;
            if (cntRes.GetRow(cntRow)) sysCount = cntRow.GetUInt(0);
        }

        PyDict* taleDict = new PyDict();
        taleDict->SetItemString("taleID", new PyInt(newID));
        taleDict->SetItemString("templateClassID", new PyInt(2));
        taleDict->SetItemString("severity", new PyInt(3)); // Vanguard on start
        PyDict* inflData = new PyDict();
        inflData->SetItemString("influence", new PyFloat(1.0));
        inflData->SetItemString("lastUpdated", new PyLong(now));
        inflData->SetItemString("decayRate", new PyFloat(0.01));
        inflData->SetItemString("graceTime", new PyInt(1800));
        taleDict->SetItemString("influenceData", new PyObject("util.KeyVal", inflData));
        taleDict->SetItemString("hasBoss", new PyBool(false));
        taleDict->SetItemString("incursedSystems", new PyList()); // populated later by NotifyClients
        PyObject* taleData = new PyObject("util.KeyVal", taleDict);

        for (auto client : clients) {
            PyIncRef(taleData);
            PyTuple* payload = new PyTuple(1);
            payload->SetItem(0, taleData);
            client->SendNotification("OnTaleStart", "clientID", payload, false);
        }
    }
    sLog.Warning("IncursionMgr", "Incursion started in constellation %u (staging: %u)", constellationID, stagingSystem);
}

void IncursionMgr::EndIncursion(uint32 incursionID)
{
    double now = GetFileTimeNow();

    // Award CONCORD LP to all participants if mothership was killed
    DBQueryResult bossRes;
    uint8 hasBoss = 0;
    if (sDatabase.RunQuery(bossRes,
        "SELECT hasBoss FROM incursions WHERE incursionID = %u", incursionID))
    {
        DBResultRow bossRow;
        if (bossRes.GetRow(bossRow))
            hasBoss = bossRow.GetUInt(0);
    }

    if (hasBoss >= 1) {
        // Award CONCORD LP to all players in incursion systems
        const std::map<uint32, SystemManager*>& systems = sEntityList.GetSystems();
        for (auto& [sysID, sysMgr] : systems) {
            if (sysMgr == nullptr) continue;
            std::vector<Client*> clients;
            sysMgr->GetClientList(clients);
            for (Client* client : clients) {
                if (client == nullptr || !client->IsInSpace()) continue;
                if (IsIncursionSystem(client->GetSystemID())) {
                    int totalLP = 10000;
                    LPService::AddLP(client->GetCharacterID(), corpCONCORD, totalLP);
                    client->SendNotifyMsg("CONCORD bonus LP awarded for incursion mothership kill.");
                    sLog.Warning("IncursionMgr", "Awarded %d CONCORD LP to %s", totalLP, client->GetName());
                }
            }
        }
    }

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE incursions SET state = 0, lastUpdated = %.0f WHERE incursionID = %u",
        now, incursionID);
    sDatabase.RunQuery(err,
        "DELETE FROM incursionSystems WHERE incursionID = %u", incursionID);

    m_activeSystems.clear();
    // Send OnTaleEnd before NotifyClients so client knows incursion is gone
    std::vector<Client*> allClients;
    sEntityList.GetClients(allClients);
    for (auto client : allClients) {
        PyTuple* payload = new PyTuple(1);
        payload->SetItem(0, new PyInt(incursionID));
        client->SendNotification("OnTaleEnd", "clientID", payload, false);
    }
    NotifyClients(incursionID);
    sLog.Warning("IncursionMgr", "Incursion %u ended (mothership%s killed)", incursionID, hasBoss>=1?"":" NOT");
}

void IncursionMgr::OnSiteCompleted(uint32 incursionID, uint32 solarSystemID, uint8 sceneType)
{
    // Allow this system to host a new site (m_activeSystems prevents re-spawn in SpawnSites)
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

        // Focus period: mothership spawns after 72h (HS), 24h (LS), 0h (NS)
        if (hasBoss == 1 && sceneType == Incursion::scenesType::headquarters) {
            DBQueryResult focusRes;
            if (sDatabase.RunQuery(focusRes,
                "SELECT UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(lastUpdated), "
                "  (SELECT AVG(s.security) FROM mapSolarSystems s "
                "   JOIN incursionSystems isys ON s.solarSystemID=isys.solarSystemID "
                "   WHERE isys.incursionID=%u) as avgSec "
                "FROM incursions WHERE incursionID=%u", incursionID, incursionID))
            {
                DBResultRow focusRow;
                if (focusRes.GetRow(focusRow)) {
                    double elapsedHours = focusRow.GetDouble(0) / 3600.0;
                    double avgSec = focusRow.GetDouble(1);
                    uint32 minHours = (avgSec >= 0.5f) ? 72 : (avgSec >= 0.0f) ? 24 : 0;
                    if (elapsedHours < minHours) {
                        _log(COSMIC_MGR__TRACE, "IncursionMgr: Mothership focus period active (%.1f/%uh)", elapsedHours, minHours);
                        continue;
                    }
                }
            }
            SpawnMothership(incursionID, solarSystemID);
            m_activeSystems.insert(solarSystemID);
            continue;
        }

        // Pick dungeonID based on sceneType
        uint32 dungeonID = 0;
        switch (sceneType) {
            case Incursion::scenesType::vanguard:      dungeonID = 2100 + MakeRandomInt(0, 3); break;
            case Incursion::scenesType::assault:        dungeonID = 2110 + MakeRandomInt(0, 2); break;
            case Incursion::scenesType::headquarters:   dungeonID = 2120 + MakeRandomInt(0, 2); break;
            case Incursion::scenesType::staging:        dungeonID = 2130 + MakeRandomInt(0, 3); break;
            default: continue;
        }

        GPoint pos;
        pos.x = MakeRandomFloat(-1.0e12, 1.0e12);
        pos.y = MakeRandomFloat(-1.0e12, 1.0e12);
        pos.z = MakeRandomFloat(-1.0e12, 1.0e12);

        // Use MakeDungeon to spawn full site (NPCs + static objects from room definitions)
        // Original Entity-category typeIDs from SDE — slim item overrides categoryID=6 for crosshairs
        CosmicSignature sig;
        sig.systemID = solarSystemID;
        sig.sigGroupID = EVEDB::invGroups::Cosmic_Anomaly;
        sig.ownerID = sDataMgr.GetFactionCorp(factionSanshas);
        sig.dungeonType = 7; // Anomaly archetype
        sig.position = pos;
        sig.sigName = "Incursion Site";
        sig.sigTypeID = EVEDB::invTypes::CosmicAnomaly;
        sig.sigStrength = 100.0f;

        DungeonMgr* dMgr = sMgr->GetDungMgr();
        if (dMgr == nullptr)
            continue;

        if (dMgr->MakeDungeon(sig, dungeonID)) {
            // Register with AnomalyMgr using pre-built signature (bypasses AddSignal's Celestial filter)
            AnomalyMgr* anomMgr = sMgr->GetAnomMgr();
            if (anomMgr != nullptr) {
                sig.sigName = "Incursion Site";
                sig.sigStrength = 1.0f;
                sig.scanGroupID = Scanning::Group::Anomaly;
                sig.dungeonType = Dungeon::Type::Anomaly;
                anomMgr->AddSignalBySignature(sig);
            }

            m_activeSystems.insert(solarSystemID);
            sLog.Warning("IncursionMgr", "Spawned incursion site dungeonID=%u in system %u (sceneType=%u)",
                dungeonID, solarSystemID, sceneType);
        }
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

void IncursionMgr::RecordDamage(uint32 bubbleID, uint32 charID, double damage) {
    m_bubbleDamage[bubbleID][charID] += damage;
}

std::map<uint32, double>& IncursionMgr::GetBubbleDamage(uint32 bubbleID) {
    return m_bubbleDamage[bubbleID];
}

void IncursionMgr::ClearDamageData(uint32 bubbleID) {
    m_bubbleDamage.erase(bubbleID);
}

void IncursionMgr::NotifyClients(uint32 incursionID)
{
    // Notify all online clients about incursion state changes.
    // Client expects OnTaleData/OnTaleStart/OnTaleEnd/OnInfluenceUpdate
    // with full taleData structures for incursion HUD display.
    std::vector<Client*> clients;
    sEntityList.GetClients(clients);

    DBQueryResult incRes;
    if (incursionID > 0)
        sDatabase.RunQuery(incRes, "SELECT incursionID, factionID, stagingSolarSystemID, constellationID, state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate, lastUpdated FROM incursions WHERE incursionID = %u", incursionID);
    else
        sDatabase.RunQuery(incRes, "SELECT incursionID, factionID, stagingSolarSystemID, constellationID, state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate, lastUpdated FROM incursions WHERE state > 0");

    DBResultRow row;
    while (incRes.GetRow(row)) {
        uint32 id = row.GetUInt(0);
        uint32 factionID = row.GetUInt(1);
        uint32 stagingSys = row.GetUInt(2);
        uint32 constID = row.GetUInt(3);
        uint8 state = row.GetUInt(4);
        float influence = row.GetFloat(5);
        bool hasBoss = row.GetUInt(6) > 0;
        uint32 rewardGroupID = row.GetUInt(7);
        uint32 taleID = row.GetUInt(8);
        uint32 graceTime = row.GetUInt(9);
        float decayRate = row.GetFloat(10);
        int64 lastUpdated = row.GetInt64(11);

        // Build incursedSystems list for this tale
        DBQueryResult sysRes;
        sDatabase.RunQuery(sysRes, "SELECT solarSystemID, sceneType, influence FROM incursionSystems WHERE incursionID = %u", id);
        PyList* incursedSystems = new PyList();
        DBResultRow sysRow;
        while (sysRes.GetRow(sysRow)) {
            incursedSystems->AddItem(new PyInt(sysRow.GetUInt(0)));
        }

        // severity: 1=HQ, 2=Assault, 3=Vanguard, 4=Staging (from client)
        uint8 severity = 3; // default Vanguard
        switch (state) {
            case 1: severity = 4; break; // mobilizing → Staging
            case 2: severity = 3; break; // established → Vanguard
            case 3: severity = 1; break; // withdrawing → HQ
        }

        // Build influenceData sub-object
        PyDict* inflDataDict = new PyDict();
        inflDataDict->SetItemString("influence", new PyFloat(influence));
        inflDataDict->SetItemString("lastUpdated", new PyLong(lastUpdated));
        inflDataDict->SetItemString("decayRate", new PyFloat(decayRate));
        inflDataDict->SetItemString("graceTime", new PyInt(graceTime * 60)); // minutes→seconds
        PyObject* influenceData = new PyObject("util.KeyVal", inflDataDict);

        // Build taleData
        PyDict* taleDict = new PyDict();
        taleDict->SetItemString("taleID", new PyInt(taleID > 0 ? taleID : id));
        taleDict->SetItemString("templateClassID", new PyInt(2)); // 2=incursion
        taleDict->SetItemString("severity", new PyInt(severity));
        taleDict->SetItemString("influenceData", influenceData);
        taleDict->SetItemString("hasBoss", new PyBool(hasBoss));
        taleDict->SetItemString("incursedSystems", incursedSystems);
        PyObject* taleData = new PyObject("util.KeyVal", taleDict);

        // Send notifications to all clients
        for (auto client : clients) {
            // OnTaleData: per-system data with full taleData structure
            // PyIncRef before each send (SendNotification may decref).
            PyIncRef(taleData);
            PyTuple* tdPayload = new PyTuple(2);
            tdPayload->SetItem(0, new PyInt(stagingSys));
            tdPayload->SetItem(1, taleData);
            client->SendNotification("OnTaleData", "clientID", tdPayload, false);

            // OnInfluenceUpdate: influence change
            PyIncRef(influenceData);
            PyTuple* inflPayload = new PyTuple(2);
            inflPayload->SetItem(0, new PyInt(taleID > 0 ? taleID : id));
            inflPayload->SetItem(1, influenceData);
            client->SendNotification("OnInfluenceUpdate", "clientID", inflPayload, false);

            // OnTaleStart/OnTaleEnd sent separately when state transitions
        }
    }
}
