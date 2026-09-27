/**
 * @name Outpost.cpp
 *   Class for Outposts and Construction Platforms.
 *
 * @Author:           James
 * @date:   17 October 2021
 */


#include "eve-server.h"

#include "Client.h"
#include "EntityList.h"
#include "EVE_Mail.h"
#include "StaticDataMgr.h"
#include "station/Outpost.h"
#include "station/StationDB.h"
#include "station/StationDataMgr.h"
#include "system/Damage.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

OutpostSE::OutpostSE(StationItemRef station, EVEServiceManager &services, SystemManager* system)
: StationSE(station, services, system),
  m_conquerable(true),
  m_reinforceState(0),
  m_reinforceEnd(0)
{
}

void OutpostSE::SpawnStationService(Client* pClient, StationData stData, uint32 serviceType)
{
    // Station service entities will be implemented later
    _log(POS__DEBUG, "Outpost::SpawnStationService(%u) called for %s(%u) — stub.", serviceType, GetName(), m_self->itemID());
}

bool OutpostSE::CheckReinforce()
{
    if (!m_conquerable)
        return false;

    // Check if current reinforcement timer has expired
    if (m_reinforceState > 0 && GetFileTimeNow() >= m_reinforceEnd) {
        // Timer expired — advance state
        if (m_reinforceState == 1) {
            // Shield reinforced → armor reinforced
            m_reinforceState = 2;
            m_reinforceEnd = GetFileTimeNow() + 24 * EvE::Time::Hour;
            _log(POS__MESSAGE, "Outpost %s(%u): Shield reinforcement expired, entering Armor Reinforced.", GetName(), m_self->itemID());
            return true;
        } else if (m_reinforceState == 2) {
            // Armor reinforced → vulnerable (can be captured now)
            m_reinforceState = 0;
            m_reinforceEnd = 0;
            _log(POS__MESSAGE, "Outpost %s(%u): Armor reinforcement expired, now vulnerable.", GetName(), m_self->itemID());
            return true;
        }
    }

    // Called when the outpost is about to be destroyed — check if it should reinforce instead
    if (m_reinforceState == 0) {
        // Not in reinforcement → enter shield reinforcement
        m_reinforceState = 1;
        m_reinforceEnd = GetFileTimeNow() + 24 * EvE::Time::Hour;
        _log(POS__MESSAGE, "Outpost %s(%u): Entering Shield Reinforced (24h).", GetName(), m_self->itemID());
        return true;
    }

    if (m_reinforceState == 1) {
        // Already in shield reinforced, timer hasn't expired yet — ignore damage
        return true;
    }

    if (m_reinforceState == 2) {
        // Armor reinforced and timer expired → capture instead of destroy
        return false; // Let it be destroyed — Killed() will handle capture
    }

    return false;
}

void OutpostSE::Killed(Damage& damage)
{
    if (m_conquerable && m_reinforceState == 2 && GetFileTimeNow() >= m_reinforceEnd) {
        // Armor reinforcement expired → capture instead of destroy
        Capture(damage);
        return;
    }
    // Normal kill
    SystemEntity::Killed(damage);
}

void OutpostSE::Capture(Damage& damage)
{
    _log(POS__MESSAGE, "Outpost %s(%u): Captured!", GetName(), m_self->itemID());

    // Determine attacker
    uint32 newCorpID = 0;
    uint32 newAllianceID = 0;
    SystemEntity* killer = damage.srcSE;

    if (killer != nullptr) {
        if (killer->HasPilot()) {
            Client* pClient = killer->GetPilot();
            newCorpID = pClient->GetCorporationID();
            newAllianceID = pClient->GetAllianceID();
        } else if (killer->IsDroneSE()) {
            Client* pClient = sEntityList.FindClientByCharID(killer->GetSelf()->ownerID());
            if (pClient != nullptr) {
                newCorpID = pClient->GetCorporationID();
                newAllianceID = pClient->GetAllianceID();
            }
        }
    }

    if (newCorpID == 0) {
        _log(POS__ERROR, "Outpost %s(%u): Capture failed — cannot determine attacker.", GetName(), m_self->itemID());
        return;
    }

    // Transfer ownership in DB
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE staStations SET corporationID = %u WHERE stationID = %u",
        newCorpID, m_self->itemID());

    // Restore station to full health
    m_self->SetAttribute(AttrShieldCharge, m_self->GetAttribute(AttrShieldCapacity));
    m_self->SetAttribute(AttrArmorDamage, EvilZero);
    m_self->SetAttribute(AttrDamage, EvilZero);

    m_reinforceState = 0;
    m_reinforceEnd = 0;

    // Notify
    PyTuple* data = new PyTuple(2);
        PySetItemRelease(data, 0, new PyInt(m_system->GetID()));
        data->SetItem(1, PyStatic.NewNone());

    std::vector<Client*> clients;
    sEntityList.GetClients(clients);
    for (auto* c : clients) {
        if (c != nullptr)
            c->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }

    _log(POS__MESSAGE, "Outpost %s(%u): Captured by corp %u (ally %u).", GetName(), m_self->itemID(), newCorpID, newAllianceID);
}

// Insert one station service ITEM (group 874) at the station's EXACT coordinates
// with flagStructureActive, so CorpStationMgr::GetStationServiceStates finds it
// (it joins staStations s and entity e on s.x=e.x AND s.y=e.y AND s.z=e.z).
static void InsertServiceEntity(uint32 corpID, uint32 sysID, const GPoint& pos, uint32 serviceType)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO entity (itemName, typeID, ownerID, locationID, flag, singleton, quantity, x, y, z, customInfo, isActive)"
        " VALUES ('%s', %u, %u, %u, %u, 1, 1, %f, %f, %f, 'stationService', 1)",
        sDataMgr.GetTypeName(serviceType), serviceType, corpID, sysID,
        (uint32)flagStructureActive, pos.x, pos.y, pos.z);
}

void OutpostSE::CompleteReadyOutposts()
{
    // Downtime step: construction platforms that were FILLED (InventoryBound::Build
    // marked them "outpostready:<stationType>") become finished outposts now.
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT e.itemID, e.ownerID, e.locationID, e.x, e.y, e.z, e.customInfo"
        " FROM entity e JOIN invTypes t ON t.typeID = e.typeID"
        " WHERE e.customInfo LIKE 'outpostready:%' AND t.groupID = %u",
        EVEDB::invGroups::Construction_Platform))
        return;

    static const uint32 svc[] = {
        EVEDB::invTypes::FittingService, EVEDB::invTypes::ReprocessingService,
        EVEDB::invTypes::FactoryService, EVEDB::invTypes::CloningService,
        EVEDB::invTypes::RepairService,  EVEDB::invTypes::LaboratoryService
    };

    DBResultRow row;
    uint32 done = 0;
    while (res.GetRow(row)) {
        uint32 platformID = row.GetUInt(0);
        uint32 ownerID    = row.GetUInt(1);
        uint32 sysID      = row.GetUInt(2);
        GPoint pos(row.GetDouble(3), row.GetDouble(4), row.GetDouble(5));
        std::string ci = row.GetText(6);
        uint32 stationType = (uint32)strtoul(ci.c_str() + strlen("outpostready:"), nullptr, 10);
        if (stationType == 0)
            continue;

        // Owner: the platform may be owned by a char or a corp.
        uint32 corpID = ownerID;
        {
            DBQueryResult cr; DBResultRow r;
            bool isCorp = false;
            if (sDatabase.RunQuery(cr, "SELECT 1 FROM crpCorporation WHERE corporationID = %u", ownerID) && cr.GetRow(r))
                isCorp = true;
            if (!isCorp) {
                corpID = 0;
                DBQueryResult chr;
                if (sDatabase.RunQuery(chr, "SELECT corporationID FROM chrCharacters WHERE characterID = %u", ownerID)) {
                    DBResultRow r2;
                    if (chr.GetRow(r2)) corpID = r2.GetUInt(0);
                }
            }
        }
        if (corpID == 0)
            corpID = 1;

        // System constellation/region/security + closest planet (name + orbit).
        uint32 constID = 0, regionID = 0; double sec = 0.0;
        {
            DBQueryResult sr; DBResultRow r;
            if (sDatabase.RunQuery(sr, "SELECT constellationID, regionID, security FROM mapSolarSystems WHERE solarSystemID = %u", sysID) && sr.GetRow(r)) {
                constID = r.GetUInt(0); regionID = r.GetUInt(1); sec = r.GetDouble(2);
            }
        }
        std::string planetName = "Unknown";
        uint32 orbitID = 0;
        {
            DBQueryResult pr; DBResultRow r;
            if (sDatabase.RunQuery(pr,
                "SELECT itemID, itemName FROM mapDenormalize WHERE solarSystemID = %u AND groupID = 6"
                " ORDER BY SQRT(POW(x-%f,2)+POW(y-%f,2)+POW(z-%f,2)) LIMIT 1",
                sysID, pos.x, pos.y, pos.z) && pr.GetRow(r)) {
                orbitID = r.GetUInt(0); planetName = r.GetText(1);
            }
        }

        // Same station build as InventoryBound::Build, but from the DB.
        StationData stData = StationData();
        stData.stationID = StationDB::GetNewOutpostID();
        std::string baseName = "Outpost";
        {
            DBQueryResult bres; DBResultRow brow;
            StationDB::GetStationBaseData(bres, stationType);
            while (bres.GetRow(brow)) {
                stData.dockOrientation = GVector(brow.GetDouble(0), brow.GetDouble(1), brow.GetDouble(2));
                stData.conquerable = brow.GetBool(3);
                stData.hangarGraphicID = brow.GetUInt(4);
                stData.description = brow.GetText(5);
                stData.descriptionID = brow.GetInt(6);
                stData.graphicID = brow.GetInt(7);
                stData.dockEntry = GPoint(brow.GetDouble(8), brow.GetDouble(9), brow.GetDouble(10));
                stData.operationID = brow.GetUInt(11);
                stData.dockPosition = GPoint(brow.GetDouble(8) + pos.x, brow.GetDouble(9) + pos.y, brow.GetDouble(10) + pos.z);
                baseName = brow.GetText(12);
            }
        }
        StationType* stType = StationType::Load(stationType);
        if (stType == nullptr)
            continue;

        stData.radius = stType->radius();
        stData.systemID = sysID;
        stData.constellationID = constID;
        stData.regionID = regionID;
        stData.position = pos;
        stData.security = sec;
        stData.typeID = stationType;
        stData.reprocessingHangarFlag = flagHangar;
        stData.corporationID = corpID;
        stData.orbitID = orbitID;
        stData.name = planetName + " - " + baseName;
        stData.officeRentalFee = 10000;
        stData.maxShipVolumeDockable = 50000000;
        stData.dockingCostPerVolume = 0;
        stData.officeSlots = 8;
        stData.reprocessingEfficiency = 0.5;
        stData.reprocessingStationsTake = 0.05;
        stData.serviceMask = Station::ReprocessingPlant | Station::Refinery | Station::Market
            | Station::BlackMarket | Station::StockExchange | Station::Cloning | Station::Surgery
            | Station::DNATherapy | Station::RepairFacilities | Station::Factory | Station::Laboratory
            | Station::Gambling | Station::Fitting | Station::Paintshop | Station::News | Station::Storage
            | Station::Insurance | Station::Docking | Station::OfficeRental | Station::JumpCloneFacility
            | Station::LoyaltyPointStore | Station::NavyOffices;

        stDataMgr.AddOutpost(stData);   // writes the station to the DB + data manager
        sDataMgr.AddOutpost(stData);    // static data (client cache)

        for (uint32 st : svc)
            InsertServiceEntity(corpID, sysID, pos, st);

        DBerror err;
        sDatabase.RunQuery(err, "DELETE FROM entity_attributes WHERE itemID = %u", platformID);
        sDatabase.RunQuery(err, "DELETE FROM entity WHERE itemID = %u", platformID);
        ++done;

        _log(POS__MESSAGE, "Downtime: construction platform %u completed -> outpost %s (%u).",
             platformID, stData.name.c_str(), stData.stationID);
    }

    if (done > 0)
        sLog.White("      Outpost", "Downtime: completed %u outpost(s) from ready construction platforms.", done);
}
