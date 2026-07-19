/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Zhur
    Rewrite:    Allan
*/

#include "eve-server.h"


#include "StaticDataMgr.h"
#include "map/MapData.h"
#include "map/MapService.h"
#include "EVE_Incursion.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

MapService::MapService() :
    Service("map", eAccessLevel_Character)
{
    this->Add("GetHistory", &MapService::GetHistory);
    this->Add("GetBeaconCount", &MapService::GetBeaconCount);    //ColorStarsByCynosuralFields
    this->Add("GetStationCount", &MapService::GetStationCount);    //ColorStarsByStationCount
    this->Add("GetMyExtraMapInfo", &MapService::GetMyExtraMapInfo);     //ColorStarsByCorpMates
    this->Add("GetStationExtraInfo", &MapService::GetStationExtraInfo);
    this->Add("GetSolarSystemVisits", &MapService::GetSolarSystemVisits);
    this->Add("GetLinkableJumpArrays", &MapService::GetLinkableJumpArrays);
    this->Add("GetMyExtraMapInfoAgents", &MapService::GetMyExtraMapInfoAgents);  //ColorStarsByMyAgents
    this->Add("GetSolarSystemPseudoSecurities", &MapService::GetSolarSystemPseudoSecurities);

    /**  not handled yet...these are empty calls  */
    this->Add("GetStuckSystems", &MapService::GetStuckSystems);
    this->Add("GetRecentSovActivity", &MapService::GetRecentSovActivity);
    this->Add("GetDeadspaceAgentsMap", &MapService::GetDeadspaceAgentsMap);
    this->Add("GetDeadspaceComplexMap", &MapService::GetDeadspaceComplexMap);
    this->Add("GetIncursionGlobalReport", &MapService::GetIncursionGlobalReport);
    this->Add("GetSystemsInIncursions", &MapService::GetSystemsInIncursions);    //ColorStarsByIncursions
    this->Add("GetSystemsInIncursionsGM", &MapService::GetSystemsInIncursionsGM);  //ColorStarsByIncursions
    this->Add("GetVictoryPoints", &MapService::GetVictoryPoints);
    this->Add("GetAllianceJumpBridges", &MapService::GetAllianceJumpBridges);
    this->Add("GetAllianceBeacons", &MapService::GetAllianceBeacons);
    this->Add("GetCurrentSovData", &MapService::GetCurrentSovData);
    // custom call for displaying all items in system
    this->Add ("GetCurrentEntities", &MapService::GetCurrentEntities);
}

PyResult MapService::GetCurrentEntities(PyCallArgs &call)
{
    return call.client->SystemMgr()->GetCurrentEntities();
}

PyResult MapService::GetSolarSystemVisits(PyCallArgs &call)
{
    return MapDB::GetSolSystemVisits(call.client->GetCharacterID());
}

PyResult MapService::GetMyExtraMapInfoAgents(PyCallArgs &call)
{
    return StandingDB::GetMyStandings(call.client->GetCharacterID());
}

PyResult MapService::GetMyExtraMapInfo(PyCallArgs &call)
{
    return CharacterDB::GetMyCorpMates(call.client->GetCorporationID());
}

PyResult MapService::GetBeaconCount(PyCallArgs &call)
{
    return MapDB::GetDynamicData(2, 24);
}

PyResult MapService::GetStationExtraInfo(PyCallArgs &call)
{
    return sMapData.GetStationExtraInfo();
}

PyResult MapService::GetSolarSystemPseudoSecurities(PyCallArgs &call)
{
    // cant find a call to this in client (possible old call)
    return sMapData.GetPseudoSecurities();
}

PyResult MapService::GetStationCount(PyCallArgs &call)
{
    // cached on client side.  if cache is empty, this call is made.
    return sDataMgr.GetStationCount();
}

PyResult MapService::GetHistory(PyCallArgs &call, PyInt* int1, PyInt* int2) {
    if (is_log_enabled(SERVICE__CALLS))
        sLog.Cyan( "MapService::Handle_GetHistory()", "type: %i, timeframe: %i", int1, int2 );

    return MapDB::GetDynamicData(int1->value(), int2->value());
}

PyResult MapService::GetLinkableJumpArrays(PyCallArgs &call)
{   // working
    DBQueryResult res;
    PosMgrDB::GetLinkableJumpArrays(call.client->GetCorporationID(), res);
    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        // SELECT systemID, itemID
        PyTuple* tuple = new PyTuple(2);
        tuple->SetItem(0, new PyInt(row.GetInt(0)));
        tuple->SetItem(1, new PyInt(row.GetInt(1)));
        list->AddItem(tuple);
    }

    return list;
}

/** not handled */

PyResult MapService::GetAllianceJumpBridges(PyCallArgs &call)
{
    /**     bridgesByLocation = m.GetAllianceJumpBridges()
     *      for toLocID, fromLocID in bridgesByLocation:
     */
    sLog.Warning( "MapService::Handle_GetAllianceJumpBridges()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    DBQueryResult res;
    PosMgrDB::GetAllianceJumpArrays(call.client->GetAllianceID(), res);
    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        // SELECT systemID, itemID
        PyTuple* tuple = new PyTuple(2);
        tuple->SetItem(0, new PyInt(row.GetInt(0)));
        tuple->SetItem(1, new PyInt(row.GetInt(1)));
        list->AddItem(tuple);
    }

    return list;
}

PyResult MapService::GetAllianceBeacons(PyCallArgs &call)
{
    sLog.Warning( "MapService::Handle_GetAllianceBeacons()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    // Get data directly from sovereignty manager, avoiding db hits
    return svDataMgr.GetAllianceBeacons(call.client->GetAllianceID());
}

PyResult MapService::GetCurrentSovData(PyCallArgs &call, PyRep* locationID)
{
    sLog.Warning( "MapService::Handle_GetCurrentSovData()", "size=%lu", call.tuple->size());

    if (locationID->IsNone()) {
        // Return ALL systems with sovereignty data
        DBQueryResult res;
        sDatabase.RunQuery(res,
            "SELECT s.solarSystemID, s.constellationID, s.regionID, "
            "  COALESCE(sov.corporationID, 0) as corporationID, "
            "  COALESCE(sov.allianceID, 0) as allianceID, "
            "  COALESCE(sov.factionID, 0) as factionID, "
            "  COALESCE(sov.sovereigntyLevel, 0) as sovereigntyLevel "
            "FROM mapSolarSystems s "
            "LEFT JOIN mapSystemSovereigntyInfo sov ON s.solarSystemID = sov.solarSystemID");
        return DBResultToCRowset(res);
    }
    uint32 id = locationID->AsInt()->value();
    return svDataMgr.GetCurrentSovData(id);
}
PyResult MapService::GetRecentSovActivity(PyCallArgs &call)
{
    _log(SERVICE__MESSAGE, "MapService::Handle_GetRecentSovActivity()", "size=%lu", call.tuple->size());

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT s.solarSystemID, COALESCE(s.factionID, 0) as factionID, sov.contested"
        " FROM mapSystemSovInfo sov"
        " LEFT JOIN mapSolarSystems s ON s.solarSystemID = sov.solarSystemID"))
    {
        return new PyDict();
    }
    return DBResultToCRowset(res);
}

//   DED Agent Site Report
PyResult MapService::GetDeadspaceAgentsMap(PyCallArgs &call, PyInt* languageID)
{/* no packet data
        dungeons = sm.RemoteSvc('map').GetDeadspaceAgentsMap(eve.session.languageID)
        solarSystemID, dungeonID, difficulty, dungeonName = dungeons
*/
    _log(SERVICE__MESSAGE, "MapService::Handle_GetDeadspaceAgentsMap()", "size=%lu", call.tuple->size());
    PyRep *result = new PyDict();

    return result;
}

//  DED Deadspace Report
//22:37:54 L MapService::Handle_GetDeadspaceComplexMap(): size= 1
PyResult MapService::GetDeadspaceComplexMap(PyCallArgs &call, PyInt* languageID)
{/* no packet data
        dungeons = sm.RemoteSvc('map').GetDeadspaceComplexMap(eve.session.languageID)
        solarSystemID, dungeonID, difficulty, dungeonName = dungeons

        get this data from managerDB.GetAnomalyList(DBQueryResult& res)
        res =  sysSignatures (sigID,sigItemID,dungeonType,sigName,systemID,sigTypeID,sigGroupID,scanGroupID,scanAttributeID,x,y,z)
*/
    _log(SERVICE__MESSAGE, "MapService::Handle_GetDeadspaceComplexMap()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);
    PyRep *result = new PyDict();

    return result;
}

PyResult MapService::GetSystemsInIncursions(PyCallArgs &call) {
    DBQueryResult res;
    PyList* list = new PyList();
    if (sDatabase.RunQuery(res,
        "SELECT solarSystemID, sceneType FROM incursionSystems "
        "WHERE sceneType IN (%u, %u)",
        Incursion::scenesType::staging, Incursion::scenesType::vanguard))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            PyTuple* tuple = new PyTuple(2);
            tuple->SetItem(0, new PyInt(row.GetInt(0)));
            tuple->SetItem(1, new PyInt(row.GetInt(1)));
            list->AddItem(tuple);
        }
    }
    return list;
}

PyResult MapService::GetSystemsInIncursionsGM(PyCallArgs &call) {
    DBQueryResult res;
    PyList* list = new PyList();
    if (sDatabase.RunQuery(res,
        "SELECT solarSystemID, sceneType FROM incursionSystems"))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            PyTuple* tuple = new PyTuple(2);
            tuple->SetItem(0, new PyInt(row.GetInt(0)));
            tuple->SetItem(1, new PyInt(row.GetInt(1)));
            list->AddItem(tuple);
        }
    }
    return list;
}

PyResult MapService::GetIncursionGlobalReport(PyCallArgs &call) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT incursionID, factionID, stagingSolarSystemID, constellationID, state, "
        "influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate, lastUpdated"
        " FROM incursions WHERE state > 0"))
    {
        return new PyList();
    }

    PyList* report = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("incursionID",            new PyInt(row.GetUInt(0)));
        entry->SetItemString("factionID",              new PyInt(row.GetUInt(1)));
        entry->SetItemString("stagingSolarSystemID",   new PyInt(row.GetUInt(2)));
        entry->SetItemString("constellationID",        new PyInt(row.GetUInt(3)));
        entry->SetItemString("state",                  new PyInt(row.GetUInt(4)));
        entry->SetItemString("influence",              new PyFloat(row.GetDouble(5)));
        entry->SetItemString("hasBoss",                new PyInt(row.GetUInt(6)));
        entry->SetItemString("rewardGroupID",          new PyInt(row.GetUInt(7)));
        entry->SetItemString("taleID",                 new PyInt(row.GetUInt(8)));
        entry->SetItemString("graceTime",              new PyFloat(static_cast<double>(row.GetUInt(9))));
        entry->SetItemString("decayRate",              new PyFloat(row.GetDouble(10)));
        entry->SetItemString("lastUpdated",            new PyLong(row.GetInt64(11)));
        report->AddItem(new PyObject("util.KeyVal", entry));
    }

    return report;
}

//   factional warfare shit
//https://wiki.eveonline.com/en/wiki/Victory_Points_and_Command_Bunker
PyResult MapService::GetVictoryPoints(PyCallArgs &call)
{/**
    Client usage: facWarMap.GetVictoryPoints(factionID, viewmode)
    Returns dict of solarsystemID → {solarSystemID, factionID, victoryPoints, threshold}
    where victoryPoints = 24h accumulated VP, threshold = VP needed to flip system
    */
    uint32 factionID = 0;
    uint32 viewMode = 0;
    if (call.byname.find("factionID") != call.byname.end())
        factionID = PyRep::IntegerValueU32(call.byname["factionID"]);
    if (call.byname.find("viewmode") != call.byname.end())
        viewMode = PyRep::IntegerValueU32(call.byname["viewmode"]);

    DBQueryResult res;
    // Return current FW system state: occupier holds the system (= highest VP)
    // VP values are 0 since we don't track real-time VP accumulation yet
    const char* query = factionID > 0
        ? "SELECT fws.systemID, fws.factionID, fws.occupierID"
          " FROM facWarSystems fws"
          " WHERE fws.factionID = %u OR fws.occupierID = %u"
        : "SELECT fws.systemID, fws.factionID, fws.occupierID FROM facWarSystems fws";

    if (!sDatabase.RunQuery(res, query, factionID, factionID))
        return new PyDict();

    PyDict* result = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 sysID    = row.GetUInt(0);
        uint32 facID    = row.GetUInt(1);
        uint32 occID    = row.GetUInt(2);
        uint32 threshold = 5000;

        PyDict* entry = new PyDict();
        entry->SetItemString("solarSystemID", new PyInt(sysID));
        entry->SetItemString("factionID", new PyInt(facID));
        entry->SetItemString("victoryPoints", new PyInt(0));
        entry->SetItemString("threshold", new PyInt(threshold));
        result->SetItem(new PyInt(sysID), entry);
    }
    return result;
}


PyResult MapService::GetStuckSystems(PyCallArgs &call)
{
    // cant find a call to this in client (possible old call)
    sLog.Warning( "MapService::Handle_GetStuckSystems()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    uint8 none = 0;

    PyTuple* res = NULL;
    PyTuple* tuple0 = new PyTuple( 1 );

    tuple0->items[ 0 ] = new PyInt( none );

    res = tuple0;

    return res;
}

