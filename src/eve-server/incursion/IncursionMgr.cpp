#include "eve-server.h"

#include "incursion/IncursionMgr.h"
#include "incursion/IncursionService.h"
#include "EVE_Incursion.h"
#include "EVEDBUtils.h"

IncursionMgr::IncursionMgr()
    : m_timer(60000)  // check every 60s
{
}

void IncursionMgr::Process()
{
    if (!m_timer.Check())
        return;

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

    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO incursions (factionID, stagingSolarSystemID, constellationID, regionID, "
        "state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate, lastUpdated) "
        "VALUES (%u, %u, %u, %u, 1, 1.0, 0, 0, 0, 30, 0.01, %.0f)",
        factionID, stagingSystem, constellationID, regionID, now);

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

    NotifyClients(incursionID);
    sLog.Warning("IncursionMgr", "Incursion %u ended", incursionID);
}

void IncursionMgr::OnSiteCompleted(uint32 incursionID, uint32 solarSystemID, uint8 sceneType)
{
    // Reduce influence based on site type
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
    // Sites are spawned on-demand when a player enters an incursion system
    // The DungeonMgr handles this via the incursion archetype
    // This method ensures dungeon definitions are loaded for incursion systems
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT solarSystemID, sceneType FROM incursionSystems WHERE incursionID = %u",
        incursionID);
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
