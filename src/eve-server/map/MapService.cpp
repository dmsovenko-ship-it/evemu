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

PyResult MapService::GetCurrentSovData(PyCallArgs &call, PyInt* locationID)
{/**
    data = sm.RemoteSvc('map').GetCurrentSovData(constellationID)
    returns locationID, ?
    return sm.RemoteSvc('map').GetCurrentSovData(locationID)
    */
    sLog.Warning( "MapService::Handle_GetCurrentSovData()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    return svDataMgr.GetCurrentSovData(locationID->value());
}
PyResult MapService::GetRecentSovActivity(PyCallArgs &call)
{
    /** @todo will have to make db table for this one.  */
    /*
     *    data = sm.RemoteSvc('map').GetRecentSovActivity()
     *    changes = []
     *    resultMap = {}
     *    toPrime = set()
     *    for item in data:
     *        if item.stationID is None:
     *            if bool(changeMode & mapcommon.SOV_CHANGES_SOV_GAIN) and item.oldOwnerID is None:
     *                changes.append((item.solarSystemID, 'UI/Map/StarModeHandler/sovereigntySovGained', (None, item.ownerID)))
     *                toPrime.add(item.ownerID)
     *            elif bool(changeMode & mapcommon.SOV_CHANGES_SOV_LOST) and item.ownerID is None:
     *                changes.append((item.solarSystemID, 'UI/Map/StarModeHandler/sovereigntySovLost', (item.oldOwnerID, None)))
     *                toPrime.add(item.oldOwnerID)
     *        elif bool(changeMode & mapcommon.SOV_CHANGES_SOV_GAIN) and item.oldOwnerID is None:
     *            changes.append((item.solarSystemID, 'UI/Map/StarModeHandler/sovereigntyNewOutpost', (None, item.ownerID)))
     *            toPrime.add(item.ownerID)
     *        elif bool(changeMode & mapcommon.SOV_CHANGES_SOV_GAIN) and item.ownerID is not None:
     *            changes.append((item.solarSystemID, 'UI/Map/StarModeHandler/sovereigntyConqueredOutpost', (item.ownerID, item.oldOwnerID)))
     *            toPrime.add(item.ownerID)
     *            toPrime.add(item.oldOwnerID)
     *
     */

    PyDict* result = new PyDict();

    return result;
}

//   DED Agent Site Report
PyResult MapService::GetDeadspaceAgentsMap(PyCallArgs &call, PyInt* languageID)
{/* no packet data
        dungeons = sm.RemoteSvc('map').GetDeadspaceAgentsMap(eve.session.languageID)
        solarSystemID, dungeonID, difficulty, dungeonName = dungeons
*/
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
    sLog.Warning( "MapService::Handle_GetDeadspaceComplexMap()", "size=%lu", call.tuple->size());
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
        "SELECT stagingSolarSystemID, state, influence, hasBoss, rewardGroupID, taleID, "
        "graceTime, decayRate, lastUpdated FROM incursions WHERE state > 0"))
    {
        return new PyList();
    }

    PyList* report = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("stagingSolarSystemID", new PyInt(row.GetUInt(0)));
        entry->SetItemString("state",                new PyInt(row.GetUInt(1)));
        entry->SetItemString("influence",            new PyFloat(row.GetDouble(2)));
        entry->SetItemString("hasBoss",              new PyInt(row.GetUInt(3)));
        entry->SetItemString("rewardGroupID",        new PyInt(row.GetUInt(4)));
        entry->SetItemString("taleID",               new PyInt(row.GetUInt(5)));
        entry->SetItemString("graceTime",            new PyFloat(static_cast<double>(row.GetUInt(6))));
        entry->SetItemString("decayRate",            new PyFloat(row.GetDouble(7)));
        entry->SetItemString("lastUpdated",          new PyLong(row.GetInt64(8)));
        report->AddItem(new PyObject("util.KeyVal", entry));
    }

    return report;
}

//   factional warfare shit
//https://wiki.eveonline.com/en/wiki/Victory_Points_and_Command_Bunker
PyResult MapService::GetVictoryPoints(PyCallArgs &call)
{/**           factionID, viewmode, solarsystemid, threshold, current in oldhistory.iteritems():
                 */
    sLog.Warning( "MapService::Handle_GetVictoryPoints()", "size=%lu", call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    return PyStatic.NewNone();
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

