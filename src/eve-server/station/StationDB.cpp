/**
 * @name StationDB.cpp
 *      database methods for station data
 *
 * @author Allan
 * @date 14 December 2017
 *
 */


#include "eve-server.h"

#include "station/StationDB.h"
#include "station/StationDataMgr.h"


uint32 StationDB::CreateOffice(ItemData& idata, OfficeData& odata)
{
    uint32 uid = 0;
    DBerror err;
    if (!sDatabase.RunQueryLID(err, uid,
        "INSERT INTO staOffices (name, corporationID, stationID, officeFolderID, flag, solarSystemID, typeID, stationTypeID, lockDown, rentalFee, expiryDateTime)"
        " VALUES"
        " ('%s',%u,%u,%u,%u,%u, 27, %u,%u,%u,%lli)",
        odata.name.c_str(), odata.corporationID, odata.stationID, odata.folderID, idata.flag, stDataMgr.GetStationSystemID(odata.stationID), odata.typeID, odata.lockDown, odata.rentalFee, odata.expiryTime)
    )
        codelog(DATABASE__ERROR, "Error in CreateOffice query: %s", err.c_str());

    return (odata.officeID = uid);
}

void StationDB::UpdateOfficeData(OfficeData& data)
{
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE staOffices SET lockDown = %u, rentalFee = %u, expiryDateTime = %lli)", data.lockDown, data.rentalFee, data.expiryTime);
}

int32 StationDB::GetOfficeCount(uint32 corpID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT COUNT(itemID) FROM staOffices WHERE corporationID = %u ", corpID)) {
        codelog(CORP__DB_ERROR, "Error in query: %s", res.error.c_str());
        return 0;
    }

    DBResultRow row;
    if (!res.GetRow(row)) {
        codelog(CORP__DB_WARNING, "Unable to find corporation's office data (%u)", corpID);
        return 0;
    }
    return row.GetInt(0);
}

PyRep* StationDB::GetOffices(uint32 stationID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        " corporationID, itemID, officeFolderID"
        " FROM staOffices"
        " WHERE stationID = %u", stationID)
    )
        codelog(DATABASE__ERROR, "Error in GetOffices query: %s", res.error.c_str());

    return DBResultToCRowset(res);
}

bool StationDB::GetOfficeData(uint32 officeID, OfficeData& odata)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        " corporationID, stationID, typeID, lockDown, rentalFee, expiryDateTime, officeFolderID"
        " FROM staOffices"
        " WHERE itemID = %u", officeID)
    ) {
        codelog(DATABASE__ERROR, "Error in GetOfficeData query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if (!res.GetRow(row))
        return false;

    odata.officeID      = officeID;
    odata.corporationID = row.GetInt(0);
    odata.stationID     = row.GetInt(1);
    odata.typeID        = row.GetInt(2);
    odata.lockDown      = row.GetBool(3);
    odata.rentalFee     = row.GetInt64(4);
    odata.expiryTime    = row.GetInt64(5);
    odata.folderID      = row.GetInt(6);
    return true;
}


void StationDB::GetStationOfficeData(DBQueryResult& res)
{
    if (!sDatabase.RunQuery(res,
        "SELECT"
        " itemID, corporationID, stationID, typeID, lockDown, rentalFee, expiryDateTime, officeFolderID"
        " FROM staOffices")
    )
        codelog(DATABASE__ERROR, "Error in GetStationOfficeData query: %s", res.error.c_str());
}

PyRep* StationDB::GetStationOfficeIDs(uint32 locationID, uint32 corpID, const char* key)
{
    DBQueryResult res;
    if (sDataMgr.IsStation(locationID)) {
        sDatabase.RunQuery(res, "SELECT itemID AS officeID, stationID, officeFolderID"
        " FROM staOffices WHERE stationID = %u", locationID);
    } else if (IsOfficeFolder(locationID)) {
        sDatabase.RunQuery(res, "SELECT itemID AS officeID, stationID, officeFolderID"
        " FROM staOffices WHERE officeFolderID = %u",
            locationID - STATION_OFFICE_OFFSET);
    } else if (IsOfficeID(locationID)) {
        sDatabase.RunQuery(res, "SELECT itemID AS officeID, stationID, officeFolderID"
        " FROM staOffices WHERE itemID = %u", locationID);
    } else {
        _log(CORP__DB_ERROR, "StationDB::GetStationOfficeIDs got invalid locationID %u", locationID);
    }

    DBResultRow row;
    if (res.GetRow(row))
        return DBRowToRow(row);

    return nullptr;

    //return DBResultToCIndexedRowset(res, key);
    //return DBResultToIndexRowset(res, key);
    //return DBResultToCRowset(res);
    //return DBResultToRowList(res);
    //return DBResultToRowset(res);
    //return DBResultToPackedRowDict(res, key);
}

void StationDB::GetStationData(DBQueryResult& res)
{
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "  s.stationID, s.x, s.y, s.z, st.dockOrientationX, st.dockOrientationY, st.dockOrientationZ,"
        "  s.dockingCostPerVolume, s.maxShipVolumeDockable, s.officeSlots, s.officeRentalCost,"
        "  s.operationID, s.stationTypeID, s.corporationID, s.stationName,"
        "  s.reprocessingStationsTake, s.reprocessingEfficiency, s.reprocessingHangarFlag, st.conquerable, st.hangarGraphicID,"
        "  m.orbitID, m.radius, m.security, o.description, o.descriptionID, t.graphicID, s.solarSystemID, s.constellationID, s.regionID,"
        "  st.dockEntryX, st.dockEntryY, st.dockEntryZ"
        " FROM staStations AS s"
        "  LEFT JOIN staStationTypes AS st USING (stationTypeID)"
        "  LEFT JOIN invTypes AS t ON t.typeID = s.stationTypeID"
        "  LEFT JOIN staOperations AS o ON o.operationID = s.operationID"
        "  LEFT JOIN mapDenormalize AS m ON m.itemID = s.stationID")
        )
        codelog(DATABASE__ERROR, "Error in GetStationData query: %s", res.error.c_str());
}

void StationDB::GetStationBaseData(DBQueryResult& res, uint32 typeID)
{
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "  st.dockOrientationX, st.dockOrientationY, st.dockOrientationZ,"
        "  st.conquerable, st.hangarGraphicID,"
        "  o.description, o.descriptionID, t.graphicID,"
        "  st.dockEntryX, st.dockEntryY, st.dockEntryZ, o.operationID, o.operationName"
        " FROM staStationTypes AS st"
        "  LEFT JOIN invTypes AS t ON t.typeID = st.stationTypeID"
        "  LEFT JOIN staOperations AS o ON o.operationName = t.typeName"
        " COLLATE utf8_general_ci"
        " WHERE st.stationTypeID=%u", typeID)
        )
        codelog(DATABASE__ERROR, "Error in GetStationData query: %s", res.error.c_str());
}

void StationDB::GetStationSystem(DBQueryResult& res)
{
    if (!sDatabase.RunQuery(res, "SELECT stationID, solarSystemID FROM staStations"))
        codelog(DATABASE__ERROR, "Error in GetStationSystem query: %s", res.error.c_str());
}

void StationDB::GetStationRegion(DBQueryResult& res)
{
    if (!sDatabase.RunQuery(res, "SELECT stationID, regionID FROM staStations"))
        codelog(DATABASE__ERROR, "Error in GetStationRegion query: %s", res.error.c_str());
}

void StationDB::GetStationConstellation(DBQueryResult& res)
{
    if (!sDatabase.RunQuery(res, "SELECT stationID, constellationID FROM staStations"))
        codelog(DATABASE__ERROR, "Error in GetStationConstellation query: %s", res.error.c_str());
}

void StationDB::GetOperationServiceIDs(DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT operationID, serviceID FROM staOperationServices");
}

void StationDB::LoadOffices(OwnerData& od, std::vector< uint32 >& into)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT itemID FROM staOffices WHERE stationID = %u", od.locID)) {
        codelog(DATABASE__ERROR, "Error in LoadOffices query: %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while (res.GetRow(row))
        into.push_back(row.GetInt(0));
}

void StationDB::CreateOutpost(StationData &data)
{
    // Add data to mapDenormalize table
    DBerror err;
    if (!sDatabase.RunQuery(err,
        "INSERT INTO mapDenormalize (itemID,typeID,groupID,solarSystemID,constellationID,regionID,orbitID,x,y,z,radius,itemName,itemNameID,security)"
        " VALUES"
        " (%u,%u,%u,%u,%u,%u,%u,%f,%f,%f,%f,'%s',0,%f)",
        data.stationID,            //itemID
        data.typeID,               //typeID
        EVEDB::invGroups::Station, //groupID
        data.systemID,             //solarSystemID
        data.constellationID,      //constellationID
        data.regionID,             //regionID
        data.orbitID,              //orbitID
        data.position.x,           //x
        data.position.y,           //y
        data.position.z,           //z
        data.radius,               //radius
        data.name.c_str(),         //itemName
        data.security              //security
    )) {
        codelog(DATABASE__ERROR, "Error in CreateStation query: %s", err.c_str());
    }

    if (!sDatabase.RunQuery(err,
        "INSERT INTO staStations (stationID,security,dockingCostPerVolume,maxShipVolumeDockable,officeSlots,officeRentalCost,operationID,stationTypeID,corporationID,solarSystemID,constellationID,regionID,stationName,x,y,z,reprocessingEfficiency,reprocessingStationsTake,reprocessingHangarFlag)"
        " VALUES"
        " (%u,%f,%f,%u,%u,%u,%u,%u,%u,%u,%u,%u,'%s',%f,%f,%f,%f,%f,%u)",
        data.stationID,                //itemID
        data.security,                 //security
        data.dockingCostPerVolume,     //dockingCostPerVolume
        data.maxShipVolumeDockable,    //maxShipVolumeDockable
        data.officeSlots,              //officeSlots
        data.officeRentalFee,          //officeRentalCost
        data.operationID,              //operationID
        data.typeID,                   //stationTypeID
        data.corporationID,            //corporationID
        data.systemID,                 //solarSystemID
        data.constellationID,          //constellationID
        data.regionID,                 //regionID
        data.name.c_str(),             //stationName
        data.position.x,               //x
        data.position.y,               //y
        data.position.z,               //z
        data.reprocessingEfficiency,   //reprocessingEfficiency
        data.reprocessingStationsTake, //reprocessingStationsTake
        flagHangar                     //reprocessingHangarFlag
    )) {
        codelog(DATABASE__ERROR, "Error in CreateStation query: %s", err.c_str());
    }
}

// This function gets the next available outpostID from the staStation table for use during outpost construction
uint32 StationDB::GetNewOutpostID()
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT stationID FROM staStations WHERE stationID>61000000 ORDER BY stationID DESC LIMIT 1")) {
        codelog(DATABASE__ERROR, "Error in GetNewOutpostID query: %s", res.error.c_str());
        return 0;
    }

    uint32 newID(0);
    DBResultRow row;
    while (res.GetRow(row)) {
        newID = row.GetUInt(0) + 1;
    }

    // If there is no existing outpost in the db, we will use the base value
    if (newID == 0) {
        return 61000000;
    }

    return newID;
}

void StationDB::GetStationServiceStates(uint32 stationID, DBQueryResult& res)
{
    // Send a rowset of the following values:
    /*
    [PyString "solarSystemID"] = [PyInt 30001984]
    [PyString "stationID"] = [PyInt 61000012]
    [PyString "serviceID"] = [PyInt 65536]
    [PyString "stationServiceItemID"] = [PyIntegerVar 318021026]
    [PyString "isEnabled"]
    */

    sDatabase.RunQuery(res, "SELECT solarSystemID, stationID, ( "
    "    CASE  "
    "        WHEN typeID=28155 THEN 65536 "
    "        WHEN typeID=28156 THEN 16 "
    "        WHEN typeID=28157 THEN 8192 "
    "        WHEN typeID=28158 THEN 512 "
    "        WHEN typeID=28159 THEN 4096 "
    "        WHEN typeID=28166 THEN 16384 "
    "        ELSE 0 "
    "    END) AS serviceID, "
    "IFNULL(itemID, 0) AS stationServiceItemID, IF(flag=144 ,1, 0) AS isEnabled  "
    "FROM entity e "
    "  INNER JOIN staStations s ON s.x = e.x AND s.y = e.y AND s.z = e.z "
    "  WHERE s.stationID=%u AND typeID IN (28155,28156,28157,28158,28159,28166) ", stationID);
    }

void StationDB::GetStationServiceIdentifiers(DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT serviceID, serviceName, serviceNameID "
    "FROM staServices ");
}

void StationDB::GetStationManagementServiceCostModifiers(uint32 stationID, DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT serviceID, discountPerGoodStandingPoint, "
    "surchargePerBadStandingPoint "
    "FROM staOutpostServiceConfig WHERE stationID=%u", stationID);
}

void StationDB::GetStationServiceAccessRule(uint32 stationID, uint32 serviceID, DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT serviceID, serviceName, minimumStanding, minimumCharSecurity, "
    "maximumCharSecurity, minimumCorpSecurity, maximumCorpSecurity "
    "FROM staOutpostServiceConfig "
    "INNER JOIN staServices USING (serviceID) "
    "WHERE stationID=%u AND serviceID=%u", stationID, serviceID);
}

void StationDB::GetStationDetails(uint32 stationID, DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT s.stationName, s.stationID, m.orbitID, "
    "COALESCE(s.description, '') as description, "
    "s.security as security, s.dockingCostPerVolume, s.officeRentalCost, s.reprocessingStationsTake, " 
    "s.reprocessingHangarFlag, s.corporationID as ownerID, s.maxShipVolumeDockable, s.upgradeLevel "
    "FROM staStations s "
    "INNER JOIN mapDenormalize m ON s.stationID = m.itemID "
    "INNER JOIN staOperations USING (operationID) "
    "WHERE s.stationID=%u", stationID);
}

void StationDB::GetRentableItems(uint32 stationID, DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT stationID, typeID, "
    "corporationID as rentedToID, TRUE as publiclyAvailable "
    "FROM staOffices "
    "WHERE stationID=%u", stationID);
}

void StationDB::GetOwnerIDsOfClonesAtStation(uint32 stationID, uint32 corpID, DBQueryResult& res)
{
    if (corpID == -1) {
        sDatabase.RunQuery(res, "SELECT ownerID, corporationID FROM entity "
        "INNER JOIN invTypes USING (typeID) "
        "INNER JOIN chrCharacters ON entity.ownerID = chrCharacters.characterID "
        "WHERE groupID=23 AND entity.locationID=%u", stationID);
    } 
    else {
        sDatabase.RunQuery(res, "SELECT ownerID, corporationID FROM entity "
        "INNER JOIN invTypes USING (typeID) "
        "INNER JOIN chrCharacters ON entity.ownerID = chrCharacters.characterID "
        "WHERE groupID=23 AND entity.locationID=%u AND corporationID=%u", stationID, corpID);
    }

}

void StationDB::GetOutpostImprovementStaticData(DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT typeID, raceID, requiredAssemblyLineTypeID, requiredImprovementTypeID "
    "FROM staImprovements");
}

void StationDB::GetOutpostImprovements(uint32 stationID, DBQueryResult& res)
{
    sDatabase.RunQuery(res, "SELECT "
    "improvementTier2aTypeID, improvementTier3aTypeID, improvementTier1bTypeID, "
    "improvementTier1aTypeID, improvementTier2bTypeID, improvementTier1cTypeID "
    "FROM staImprovementsInstalled "
    "WHERE stationID=%u", stationID);
}

void StationDB::GetClones(uint32 ownerID, DBQueryResult& res)
{
    sDatabase.RunQuery(res,
        "SELECT itemID, typeID, locationID, itemName, customInfo, isActive"
        " FROM entity"
        " WHERE ownerID = %u AND flag = %u",
        ownerID, (uint16)flagClone);
}

void StationDB::GetImplants(uint32 ownerID, DBQueryResult& res)
{
    sDatabase.RunQuery(res,
        "SELECT itemID, typeID FROM entity"
        " WHERE ownerID = %u AND flag = %u",
        ownerID, (uint16)flagImplant);
}

uint32 StationDB::CreateClone(uint32 ownerID, uint32 typeID, uint32 locationID, const char* itemName, const char* customInfo)
{
    std::string nameEsc, customInfoEsc;
    sDatabase.DoEscapeString(nameEsc, itemName);
    sDatabase.DoEscapeString(customInfoEsc, customInfo);

    DBerror err;
    uint32 uid = 0;
    if (!sDatabase.RunQueryLID(err, uid,
        "INSERT INTO entity (itemName, typeID, ownerID, locationID, flag, quantity, customInfo, isActive)"
        " VALUES ('%s', %u, %u, %u, %u, 1, '%s', 0)",
        nameEsc.c_str(), typeID, ownerID, locationID, (uint16)flagClone, customInfoEsc.c_str()))
    {
        codelog(DATABASE__ERROR, "Error in CreateClone query: %s", err.c_str());
    }
    return uid;
}

bool StationDB::DeleteClone(uint32 cloneID)
{
    DBerror err;
    if (!sDatabase.RunQuery(err, "DELETE FROM entity WHERE itemID = %u AND flag = %u", cloneID, (uint16)flagClone)) {
        codelog(DATABASE__ERROR, "Error in DeleteClone query: %s", err.c_str());
        return false;
    }
    return true;
}

bool StationDB::GetCloneInfo(uint32 cloneID, uint32& typeID, uint32& locationID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT typeID, locationID FROM entity WHERE itemID = %u AND flag = %u",
        cloneID, (uint16)flagClone))
    {
        codelog(DATABASE__ERROR, "Error in GetCloneInfo query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if (!res.GetRow(row))
        return false;

    typeID = row.GetUInt(0);
    locationID = row.GetUInt(1);
    return true;
}

void StationDB::GetCloneImplants(uint32 cloneID, DBQueryResult& res)
{
    sDatabase.RunQuery(res,
        "SELECT typeID FROM chrJumpCloneImplants WHERE jumpCloneID = %u",
        cloneID);
}

bool StationDB::AddCloneImplant(uint32 cloneID, uint32 typeID)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        "INSERT INTO chrJumpCloneImplants (jumpCloneID, typeID) VALUES (%u, %u)",
        cloneID, typeID);
}

bool StationDB::RemoveCloneImplant(uint32 cloneID, uint32 typeID)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        "DELETE FROM chrJumpCloneImplants WHERE jumpCloneID = %u AND typeID = %u LIMIT 1",
        cloneID, typeID);
}

bool StationDB::GetActiveCloneID(uint32 ownerID, uint32& cloneID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT itemID FROM entity WHERE ownerID = %u AND flag = %u AND isActive = 1",
        ownerID, (uint16)flagClone))
    {
        codelog(DATABASE__ERROR, "Error in GetActiveCloneID query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if (!res.GetRow(row))
        return false;

    cloneID = row.GetUInt(0);
    return true;
}

void StationDB::SetCloneActive(uint32 ownerID, uint32 cloneID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE entity SET isActive = 0 WHERE ownerID = %u AND flag = %u",
        ownerID, (uint16)flagClone);
    sDatabase.RunQuery(err,
        "UPDATE entity SET isActive = 1 WHERE itemID = %u AND flag = %u",
        cloneID, (uint16)flagClone);
}

uint32 StationDB::GetClonePrice(uint32 typeID)
{
    switch (typeID) {
        case 164:   return 0;         // Clone Grade Alpha (starter, free)
        case 9941:  return 100000;    // Clone Grade Beta
        case 9942:  return 500000;    // Clone Grade Gamma
        case 9943:  return 5000000;   // Clone Grade Delta
        case 9944:  return 50000000;  // Clone Grade Epsilon
        case 9945:  return 100000000; // Clone Grade Zeta
        default:    return 1000000;   // Unknown clone grade
    }
}