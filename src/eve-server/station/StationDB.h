/**
 * @name StationDB.h
 *      database methods for station data
 *
 * @author Allan
 * @date 14 December 2017
 *
 */


#ifndef EVE_STATION_STATIONDB_H
#define EVE_STATION_STATIONDB_H

#include "ServiceDB.h"
#include "inventory/ItemType.h"


class StationDB
: public ServiceDB
{
public:
    void UpdateOfficeData(OfficeData& data);

    static PyRep* GetOffices(uint32 stationID);
    static PyRep* GetStationOfficeIDs(uint32 locationID, uint32 corpID, const char* key);

    static uint32 CreateOffice(ItemData& idata, OfficeData& odata);
    static bool GetOfficeData(uint32 officeID, OfficeData& odata);
    static void GetStationData(DBQueryResult& res);
    static void GetStationBaseData(DBQueryResult& res, uint32 typeID);
    static void GetStationSystem(DBQueryResult& res);
    static void GetStationRegion(DBQueryResult& res);
    static void GetStationOfficeData(DBQueryResult& res);
    static void GetOperationServiceIDs(DBQueryResult& res);
    static void GetStationConstellation(DBQueryResult& res);

    static int32 GetOfficeCount(uint32 corpID);

    static void LoadOffices(OwnerData &od, std::vector<uint32> &into);
    static void CreateOutpost(StationData& data);
    static uint32 GetNewOutpostID();
    static void GetStationServiceStates(uint32 stationID, DBQueryResult& res);
    static void GetStationServiceIdentifiers(DBQueryResult& res);
    static void GetStationServiceAccessRule(uint32 stationID, uint32 serviceID, DBQueryResult& res);
    static void GetStationManagementServiceCostModifiers(uint32 stationID, DBQueryResult& res);
    static void GetStationDetails(uint32 stationID, DBQueryResult& res);
    static void GetRentableItems(uint32 stationID, DBQueryResult& res);
    static void GetOwnerIDsOfClonesAtStation(uint32 stationID, uint32 corpID, DBQueryResult& res);
    static void GetOutpostImprovementStaticData(DBQueryResult& res);
    static void GetOutpostImprovements(uint32 stationID, DBQueryResult& res);

    static void GetClones(uint32 ownerID, DBQueryResult& res);
    static void GetImplants(uint32 ownerID, DBQueryResult& res);
    static void GetCloneImplants(uint32 cloneID, DBQueryResult& res);
    static bool AddCloneImplant(uint32 cloneID, uint32 typeID);
    static bool RemoveCloneImplant(uint32 cloneID, uint32 typeID);
    static uint32 CreateClone(uint32 ownerID, uint32 typeID, uint32 locationID, const char* itemName, const char* customInfo);
    static bool DeleteClone(uint32 cloneID);
    static bool GetCloneInfo(uint32 cloneID, uint32& typeID, uint32& locationID);
    static bool GetActiveCloneID(uint32 ownerID, uint32& cloneID);
    static void SetCloneActive(uint32 ownerID, uint32 cloneID);
    static uint32 GetClonePrice(uint32 typeID);

};

#endif  // EVE_STATION_STATIONDB_H
