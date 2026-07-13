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
    Author:        Zhur (outline and 3 calls)
    Rewrite:    Allan
*/

#include "eve-server.h"

#include "EntityList.h"
#include "EVE_Mail.h"
#include "standing/StandingDB.h"
#include "cache/ObjCacheService.h"
#include "faction/FactionWarMgrService.h"

/*
 * FACWAR__ERROR
 * FACWAR__WARNING
 * FACWAR__INFO
 * FACWAR__MESSAGE
 * FACWAR__TRACE
 * FACWAR__CALL
 * FACWAR__CALL_DUMP
 * FACWAR__RSP_DUMP
 */

FactionWarMgrService::FactionWarMgrService(EVEServiceManager& mgr) :
    Service("facWarMgr", eAccessLevel_SolarSystem2),
    m_manager (mgr)
{
    this->Add("GetWarFactions", &FactionWarMgrService::GetWarFactions);
    this->Add("GetFWSystems", &FactionWarMgrService::GetFWSystems);
    this->Add("GetMyCharacterRankOverview", &FactionWarMgrService::GetMyCharacterRankOverview);
    this->Add("GetMyCharacterRankInfo", &FactionWarMgrService::GetMyCharacterRankInfo);
    this->Add("GetFactionMilitiaCorporation", &FactionWarMgrService::GetFactionMilitiaCorporation);
    this->Add("GetCharacterRankInfo", &FactionWarMgrService::GetCharacterRankInfo);
    this->Add("GetFactionalWarStatus", &FactionWarMgrService::GetFactionalWarStatus);
    this->Add("GetSystemStatus", &FactionWarMgrService::GetSystemStatus);
    this->Add("IsEnemyFaction", &FactionWarMgrService::IsEnemyFaction);
    this->Add("JoinFactionAsCharacter", &FactionWarMgrService::JoinFactionAsCharacter);
    this->Add("GetCorporationWarFactionID", &FactionWarMgrService::GetCorporationWarFactionID);
    this->Add("IsEnemyCorporation", &FactionWarMgrService::IsEnemyCorporation);
    this->Add("GetSystemsConqueredThisRun", &FactionWarMgrService::GetSystemsConqueredThisRun);
    this->Add("GetFactionCorporations", &FactionWarMgrService::GetFactionCorporations);
    this->Add("JoinFactionAsCharacterRecommendationLetter", &FactionWarMgrService::JoinFactionAsCharacterRecommendationLetter);
    this->Add("JoinFactionAsAlliance", &FactionWarMgrService::JoinFactionAsAlliance);
    this->Add("JoinFactionAsCorporation", &FactionWarMgrService::JoinFactionAsCorporation);
    this->Add("GetStats_FactionInfo", &FactionWarMgrService::GetStats_FactionInfo);
    this->Add("GetStats_TopAndAllKillsAndVPs", &FactionWarMgrService::GetStats_TopAndAllKillsAndVPs);
    this->Add("GetStats_Character", &FactionWarMgrService::GetStats_Character);
    this->Add("GetStats_Alliance", &FactionWarMgrService::GetStats_Alliance);
    this->Add("GetStats_Militia", &FactionWarMgrService::GetStats_Militia);
    this->Add("GetStats_CorpPilots", &FactionWarMgrService::GetStats_CorpPilots);
    this->Add("LeaveFactionAsAlliance", &FactionWarMgrService::LeaveFactionAsAlliance);
    this->Add("LeaveFactionAsCorporation", &FactionWarMgrService::LeaveFactionAsCorporation);
    this->Add("WithdrawJoinFactionAsAlliance", &FactionWarMgrService::WithdrawJoinFactionAsAlliance);
    this->Add("WithdrawJoinFactionAsCorporation", &FactionWarMgrService::WithdrawJoinFactionAsCorporation);
    this->Add("WithdrawLeaveFactionAsAlliance", &FactionWarMgrService::WithdrawLeaveFactionAsAlliance);
    this->Add("WithdrawLeaveFactionAsCorporation", &FactionWarMgrService::WithdrawLeaveFactionAsCorporation);
    this->Add("RefreshCorps", &FactionWarMgrService::RefreshCorps);

    this->m_cache = this->m_manager.Lookup <ObjCacheService>("objectCaching");
}

PyResult FactionWarMgrService::GetWarFactions(PyCallArgs &call) {
    ObjectCachedMethodID method_id(GetName().c_str(), "GetWarFactions");

    if (!this->m_cache->IsCacheLoaded(method_id)) {
        PyRep *res = m_db.GetWarFactions();
        if (res == NULL)
            return nullptr;
        this->m_cache->GiveCache(method_id, &res);
    }

    return this->m_cache->MakeObjectCachedMethodCallResult(method_id);
}

PyResult FactionWarMgrService::GetFWSystems(PyCallArgs& call)
{
    /*
      [PySubStream 3625 bytes]
        [PyDict 171 kvp]
          [PyInt 30002813]
          [PyDict 2 kvp]
            [PyString "occupierID"]
            [PyInt 500001]
            [PyString "factionID"]
            [PyInt 500001]
          [PyInt 30005295]
          [PyDict 2 kvp]
            [PyString "occupierID"]
            [PyInt 500004]
            [PyString "factionID"]
            [PyInt 500004]
            */
    ObjectCachedMethodID method_id( GetName().c_str(), "GetFacWarSystems" );

    if ( !this->m_cache->IsCacheLoaded( method_id ) )
    {
        PyRep* res = m_db.GetFacWarSystems();
        if ( res == NULL )
            return nullptr;

        this->m_cache->GiveCache( method_id, &res );
    }

    return this->m_cache->MakeObjectCachedMethodCallResult( method_id );
}

/**     ***********************************************************************
 * @note   these below are partially coded
 */

PyResult FactionWarMgrService::GetMyCharacterRankOverview(PyCallArgs& call) {
    /**
            [PySubStream 122 bytes]
              [PyObjectEx Type2]
                [PyTuple 2 items]
                  [PyTuple 1 items]
                    [PyToken dbutil.CRowset]
                  [PyDict 1 kvp]
                    [PyString "header"]
                    [PyObjectEx Normal]
                      [PyTuple 2 items]
                        [PyToken blue.DBRowDescriptor]
                        [PyTuple 1 items]
                          [PyTuple 4 items]
                            [PyTuple 2 items]
                              [PyString "currentRank"]
                              [PyInt 3]
                            [PyTuple 2 items]
                              [PyString "highestRank"]
                              [PyInt 3]
                            [PyTuple 2 items]
                              [PyString "factionID"]
                              [PyInt 3]
                            [PyTuple 2 items]
                              [PyString "lastModified"]
                              [PyInt 64]
     */

// will need data from DB...
  util_Rowset rs;

    rs.header.push_back( "currentRank" );
    rs.header.push_back( "highestRank" );
    rs.header.push_back( "factionID" );
    rs.header.push_back( "lastModified" );

    return rs.Encode();
}

PyResult FactionWarMgrService::GetMyCharacterRankInfo(PyCallArgs& call) {
  _log(FACWAR__CALL, "FacWarMgr::Handle_GetMyCharacterRankInfo() size=%lli", call.tuple->size());
  call.Dump(FACWAR__CALL_DUMP);
  util_Rowset rs;

    rs.header.push_back( "currentRank" );
    rs.header.push_back( "highestRank" );
    rs.header.push_back( "factionID" );
    rs.header.push_back( "lastModified" );

    return rs.Encode();
}

PyResult FactionWarMgrService::GetFactionMilitiaCorporation(PyCallArgs &call, PyInt* factionID) {
    /* 05:39:07 [SvcCall] Service facWarMgr: calling GetFactionMilitiaCorporation
     * 05:39:07 FactionWarMgrService::Handle_GetFactionMilitiaCorporation(): size= 1
     * 05:39:07 [SvcCall]   Call Arguments:
     * 05:39:07 [SvcCall]       Tuple: 1 elements
     * 05:39:07 [SvcCall]         [ 0] Integer field: 500002
     */
  _log(FACWAR__CALL, "FacWarMgr::Handle_GetFactionMilitiaCorporation() size=%lli", call.tuple->size());
  call.Dump(FACWAR__CALL_DUMP);

    return (new PyInt(m_db.GetFactionMilitiaCorporation(factionID->value())));
}

PyResult FactionWarMgrService::GetSystemStatus(PyCallArgs &call, PyInt* solarsystemID, PyInt* warFactionID) {
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetSystemStatus()");

    // if caller has no FW faction, status is None
    if (warFactionID->value() == 0)
        return new PyInt(FacWar::SysStatus::None);

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT occupierID, factionID FROM facWarSystems"
        " WHERE systemID = %u",
        solarsystemID->value());

    DBResultRow row;
    if (!res.GetRow(row))
        return new PyInt(FacWar::SysStatus::None); // not an FW system

    uint32 occupierID = row.GetUInt(0);
    uint32 factionID = row.GetUInt(1);

    // if caller's faction occupies it → safe
    if (occupierID == (uint32)warFactionID->value())
        return new PyInt(FacWar::SysStatus::None);

    // check if the system is contested (has SBU or capture timer running)
    DBQueryResult sovRes;
    sDatabase.RunQuery(sovRes,
        "SELECT contested FROM mapSystemSovInfo WHERE solarSystemID = %u",
        solarsystemID->value());

    DBResultRow sovRow;
    bool contested = false;
    if (sovRes.GetRow(sovRow))
        contested = (sovRow.GetUInt(0) == 1);

    if (contested)
        return new PyInt(FacWar::SysStatus::Vulnerable);

    // enemy-occupied system
    return new PyInt(FacWar::SysStatus::Contested);
}

// these next two should use static data or cached data to avoid db hits
PyResult FactionWarMgrService::IsEnemyFaction(PyCallArgs &call, PyInt* enemyID, PyInt* factionID) {
    float standing = StandingDB::GetStanding(enemyID->value(), factionID->value());
    return new PyBool(standing < 0.0f);
}

PyResult FactionWarMgrService::IsEnemyCorporation(PyCallArgs &call, PyInt* enemyID, PyInt* factionID) {
    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT warFactionID FROM crpCorporations WHERE corporationID = %u", enemyID->value());
    DBResultRow row;
    if (!res.GetRow(row) or row.GetUInt(0) == 0)
        return PyStatic.NewFalse();
    float standing = StandingDB::GetStanding(row.GetUInt(0), factionID->value());
    return new PyBool(standing < 0.0f);
}

PyResult FactionWarMgrService::GetCharacterRankInfo(PyCallArgs &call, PyInt* characterID) {
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT currentRank, highestRank, factionID FROM facWarCharacters WHERE characterID = %u",
        characterID->value());
    DBResultRow row;
    if (!res.GetRow(row))
        return PyStatic.NewNone();

    util_Rowset rs;
    rs.header.push_back("currentRank");
    rs.header.push_back("highestRank");
    rs.header.push_back("factionID");
    rs.header.push_back("lastModified");
    rs.lines->AddItem(new PyInt(row.GetInt(0)));
    rs.lines->AddItem(new PyInt(row.GetInt(1)));
    rs.lines->AddItem(new PyInt(row.GetInt(2)));
    rs.lines->AddItem(new PyLong(GetFileTimeNow()));
    return rs.Encode();
}

PyResult FactionWarMgrService::GetFactionalWarStatus(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();
    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT factionID, joined, currentRank FROM facWarCharacters WHERE characterID = %u", charID);
    DBResultRow row;
    if (!res.GetRow(row))
        return PyStatic.NewNone();
    PyDict* dict = new PyDict();
        dict->SetItemString("factionID", new PyInt(row.GetInt(0)));
        dict->SetItemString("joined", new PyLong(row.GetInt64(1)));
        dict->SetItemString("currentRank", new PyInt(row.GetInt(2)));
        dict->SetItemString("status", new PyInt(1));  // 1 = enlisted, 0 = not enlisted
    return new PyObject("util.KeyVal", dict);
}

PyResult FactionWarMgrService::JoinFactionAsCharacter(PyCallArgs &call, PyInt* factionID) {
    uint32 charID = call.client->GetCharacterID();
    uint32 fID = factionID->value();

    // validate faction
    if (!m_db.IsValidFaction(fID)) {
        call.client->SendErrorMsg("Invalid faction.");
        return PyStatic.NewFalse();
    }

    // check not already in FW
    DBQueryResult curRes;
    sDatabase.RunQuery(curRes, "SELECT characterID FROM facWarCharacters WHERE characterID = %u", charID);
    if (curRes.GetRowCount() > 0) {
        call.client->SendErrorMsg("You are already enlisted in Faction Warfare.");
        return PyStatic.NewFalse();
    }

    DBerror err;
    int64 now = GetFileTimeNow();
    sDatabase.RunQuery(err,
        "INSERT INTO facWarCharacters (characterID, factionID, joined, lastActive, currentRank, highestRank)"
        " VALUES (%u, %u, %lli, %lli, 1, 1)",
        charID, fID, now, now);

    // update character's warFactionID
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = %u WHERE characterID = %u", fID, charID);

    // notification
    PyDict* data = new PyDict();
        data->SetItemString("factionID", new PyInt(fID));
        data->SetItemString("charID", new PyInt(charID));
    sEntityList.CreateNotification(charID, Notify::Types::FWCorpJoin, fID, data);

    call.client->SendNotifyMsg("You have joined the %s militia.", m_db.GetFactionName(fID).c_str());
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::GetCorporationWarFactionID(PyCallArgs &call, PyInt* corporationID) {
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetCorporationWarFactionID()");

    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT warFactionID FROM crpCorporations WHERE corporationID = %u", corporationID->value());
    DBResultRow row;
    if (res.GetRow(row) and row.GetUInt(0) > 0)
        return new PyInt(row.GetUInt(0));
    return PyStatic.NewNone();
}

PyResult FactionWarMgrService::GetSystemsConqueredThisRun(PyCallArgs &call) {
    // return list of systems that will switch at next downtime (>50% contested)
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT fws.systemID, fws.factionID AS occupierID"
        " FROM facWarSystems fws"
        " JOIN mapSystemSovInfo ssi ON ssi.solarSystemID = fws.systemID"
        " WHERE ssi.contested = 1");

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* dict = new PyDict();
            dict->SetItemString("solarSystemID", new PyInt(row.GetInt(0)));
            dict->SetItemString("occupierID", new PyInt(row.GetInt(1)));
        list->AddItem(new PyObject("util.KeyVal", dict));
    }
    return list;
}

PyResult FactionWarMgrService::GetFactionCorporations(PyCallArgs &call, PyInt* factionID) {
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT corporationID FROM crpCorporations WHERE warFactionID = %u", factionID->value());

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row))
        list->AddItem(new PyInt(row.GetInt(0)));
    return list;
}

PyResult FactionWarMgrService::JoinFactionAsCharacterRecommendationLetter(PyCallArgs &call, PyInt* factionID, PyInt* itemID) {
    //self.facWarMgr.JoinFactionAsCharacterRecommendationLetter, factionID, itemID)
    // if char standing with faction is < 0.5,
    // they can join provided they have a 'recommendation letter', typeID 30906
    // dunno if the letter is removed after joining.
    _log(FACWAR__CALL, "FacWarMgr::Handle_JoinFactionAsCharacterRecommendationLetter()");
    call.Dump(FACWAR__CALL_DUMP);

    return nullptr;
}

PyResult FactionWarMgrService::JoinFactionAsAlliance(PyCallArgs &call, PyInt* factionID) {
    // Alliance join requires vote - for now set alliance warFactionID
    uint32 allyID = call.client->GetAllianceID();
    if (allyID == 0)
        throw UserError("AllianceRequiredForAction");

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE alnAlliance SET warFactionID = %u WHERE allianceID = %u",
        factionID->value(), allyID);
    // update all member corps and characters
    sDatabase.RunQuery(err,
        "UPDATE crpCorporation SET warFactionID = %u WHERE allianceID = %u",
        factionID->value(), allyID);
    sDatabase.RunQuery(err,
        "UPDATE chrCharacters SET warFactionID = %u WHERE allianceID = %u",
        factionID->value(), allyID);

    call.client->SendNotifyMsg("Alliance has joined faction warfare.");
    PyDict* data = new PyDict();
        data->SetItemString("factionID", new PyInt(factionID->value()));
        data->SetItemString("allianceID", new PyInt(allyID));
    sEntityList.CreateNotification(allyID, Notify::Types::FWCorpJoin, factionID->value(), data);
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::JoinFactionAsCorporation(PyCallArgs &call, PyInt* factionID) {
    // corp join - director/CEO only (check role & 8192 = Director)
    if (!(call.client->GetCorpRole() & 8192))
        throw UserError("CrpAccessDenied").AddFormatValue("reason", new PyString("Only directors can join faction warfare."));

    uint32 corpID = call.client->GetCorporationID();
    uint32 fID = factionID->value();

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE crpCorporations SET warFactionID = %u WHERE corporationID = %u", fID, corpID);
    // update all corp members
    sDatabase.RunQuery(err,
        "UPDATE chrCharacters SET warFactionID = %u WHERE corporationID = %u", fID, corpID);

    call.client->SendNotifyMsg("Corporation has joined the %s militia.", m_db.GetFactionName(fID).c_str());
    PyDict* data = new PyDict();
        data->SetItemString("factionID", new PyInt(fID));
        data->SetItemString("corpID", new PyInt(corpID));
    sEntityList.CreateNotification(corpID, Notify::Types::FWCorpJoin, fID, data);
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::LeaveFactionAsCorporation(PyCallArgs &call, PyInt* factionID) {
    if (!(call.client->GetCorpRole() & 8192))
        throw UserError("CrpAccessDenied").AddFormatValue("reason", new PyString("Only directors can leave faction warfare."));

    uint32 corpID = call.client->GetCorporationID();
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = 0 WHERE corporationID = %u", corpID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = 0 WHERE corporationID = %u", corpID);

    call.client->SendNotifyMsg("Corporation has left faction warfare.");
    PyDict* data = new PyDict();
        data->SetItemString("factionID", new PyInt(factionID->value()));
        data->SetItemString("corpID", new PyInt(corpID));
    sEntityList.CreateNotification(corpID, Notify::Types::FWCorpLeave, corpID, data);
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::LeaveFactionAsAlliance(PyCallArgs &call, PyInt* factionID) {
    uint32 allyID = call.client->GetAllianceID();
    if (allyID == 0)
        throw UserError("AllianceRequiredForAction");

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE alnAlliance SET warFactionID = 0 WHERE allianceID = %u", allyID);
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = 0 WHERE allianceID = %u", allyID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = 0 WHERE allianceID = %u", allyID);

    call.client->SendNotifyMsg("Alliance has left faction warfare.");
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::WithdrawJoinFactionAsAlliance(PyCallArgs &call, PyInt* factionID) {
    uint32 allyID = call.client->GetAllianceID();
    if (allyID == 0)
        throw UserError("AllianceRequiredForAction");
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE alnAlliance SET warFactionID = 0 WHERE allianceID = %u", allyID);
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = 0 WHERE allianceID = %u", allyID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = 0 WHERE allianceID = %u", allyID);
    call.client->SendNotifyMsg("Alliance faction warfare join withdrawn.");
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::WithdrawJoinFactionAsCorporation(PyCallArgs &call, PyInt* factionID) {
    if (!(call.client->GetCorpRole() & 8192))
        throw UserError("CrpAccessDenied");
    uint32 corpID = call.client->GetCorporationID();
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = 0 WHERE corporationID = %u", corpID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = 0 WHERE corporationID = %u", corpID);
    call.client->SendNotifyMsg("Corporation faction warfare join withdrawn.");
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::WithdrawLeaveFactionAsAlliance(PyCallArgs &call, PyInt* factionID) {
    // Restore warFactionID from DB for alliance and all member corps/chars
    uint32 allyID = call.client->GetAllianceID();
    if (allyID == 0)
        throw UserError("AllianceRequiredForAction");

    // Get original factionID from any member char still in facWarCharacters
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT factionID FROM facWarCharacters fwc"
        " JOIN chrCharacters c ON fwc.characterID = c.characterID"
        " JOIN crpCorporations crp ON c.corporationID = crp.corporationID"
        " WHERE crp.allianceID = %u LIMIT 1", allyID);
    DBResultRow row;
    uint32 restoreFactionID = factionID->value();
    if (res.GetRow(row))
        restoreFactionID = row.GetUInt(0);

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE alnAlliance SET warFactionID = %u WHERE allianceID = %u", restoreFactionID, allyID);
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = %u WHERE allianceID = %u", restoreFactionID, allyID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = %u WHERE allianceID = %u", restoreFactionID, allyID);

    call.client->SendNotifyMsg("Faction warfare leave withdrawn, warFactionID restored.");
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::WithdrawLeaveFactionAsCorporation(PyCallArgs &call, PyInt* factionID) {
    if (!(call.client->GetCorpRole() & 8192))
        throw UserError("CrpAccessDenied").AddFormatValue("reason", new PyString("Only directors can withdraw leave."));

    uint32 corpID = call.client->GetCorporationID();

    // Get original factionID from any member still in facWarCharacters
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT factionID FROM facWarCharacters fwc"
        " JOIN chrCharacters c ON fwc.characterID = c.characterID"
        " WHERE c.corporationID = %u LIMIT 1", corpID);
    DBResultRow row;
    uint32 restoreFactionID = factionID->value();
    if (res.GetRow(row))
        restoreFactionID = row.GetUInt(0);

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE crpCorporations SET warFactionID = %u WHERE corporationID = %u", restoreFactionID, corpID);
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET warFactionID = %u WHERE corporationID = %u", restoreFactionID, corpID);

    call.client->SendNotifyMsg("Corporation faction warfare leave withdrawn, warFactionID restored.");
    return PyStatic.NewTrue();
}

PyResult FactionWarMgrService::GetStats_FactionInfo(PyCallArgs &call) {
    // return dict of factionID → {factionID, pilots, kills, losses, victoryPoints, systemsControlled}
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_FactionInfo()");

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "  ff.factionID,"
        "  (SELECT COUNT(*) FROM facWarCharacters WHERE factionID = ff.factionID) AS pilots,"
        "  COALESCE((SELECT SUM(kills) FROM facWarStats WHERE factionID = ff.factionID), 0) AS kills,"
        "  COALESCE((SELECT SUM(losses) FROM facWarStats WHERE factionID = ff.factionID), 0) AS losses,"
        "  COALESCE((SELECT SUM(victoryPoints) FROM facWarStats WHERE factionID = ff.factionID), 0) AS vps,"
        "  (SELECT COUNT(*) FROM facWarSystems WHERE occupierID = ff.factionID) AS systems"
        " FROM facFactions ff"
        " WHERE ff.militiaCorporationID IS NOT NULL"))
        return new PyDict();

    PyDict* result = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 factionID = row.GetUInt(0);
        PyDict* info = new PyDict();
            info->SetItemString("factionID", new PyInt(factionID));
            info->SetItemString("pilots", new PyInt(row.GetUInt(1)));
            info->SetItemString("kills", new PyInt(row.GetUInt(2)));
            info->SetItemString("losses", new PyInt(row.GetUInt(3)));
            info->SetItemString("victoryPoints", new PyFloat(row.GetDouble(4)));
            info->SetItemString("systemsControlled", new PyInt(row.GetUInt(5)));
        result->SetItem(new PyInt(factionID), info);
    }
    return result;
}

PyResult FactionWarMgrService::GetStats_TopAndAllKillsAndVPs(PyCallArgs &call) {
    // return { topKills: [...], topVPs: [...], totalKills: N, totalVPs: N }
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_TopAndAllKillsAndVPs()");

    // Top 10 by kills
    DBQueryResult killRes;
    PyList* topKills = new PyList();
    if (sDatabase.RunQuery(killRes,
        "SELECT s.characterID, c.characterName, s.kills"
        " FROM facWarStats s"
        " JOIN chrCharacters c ON s.characterID = c.characterID"
        " ORDER BY s.kills DESC"
        " LIMIT 10"))
    {
        DBResultRow row;
        while (killRes.GetRow(row)) {
            PyDict* entry = new PyDict();
                entry->SetItemString("characterID", new PyInt(row.GetUInt(0)));
                entry->SetItemString("characterName", new PyString(row.GetText(1)));
                entry->SetItemString("kills", new PyInt(row.GetUInt(2)));
            topKills->AddItem(new PyObject("util.KeyVal", entry));
        }
    }

    // Top 10 by victory points
    DBQueryResult vpRes;
    PyList* topVPs = new PyList();
    if (sDatabase.RunQuery(vpRes,
        "SELECT s.characterID, c.characterName, s.victoryPoints"
        " FROM facWarStats s"
        " JOIN chrCharacters c ON s.characterID = c.characterID"
        " ORDER BY s.victoryPoints DESC"
        " LIMIT 10"))
    {
        DBResultRow row;
        while (vpRes.GetRow(row)) {
            PyDict* entry = new PyDict();
                entry->SetItemString("characterID", new PyInt(row.GetUInt(0)));
                entry->SetItemString("characterName", new PyString(row.GetText(1)));
                entry->SetItemString("victoryPoints", new PyFloat(row.GetDouble(2)));
            topVPs->AddItem(new PyObject("util.KeyVal", entry));
        }
    }

    // Totals
    DBQueryResult totalRes;
    uint32 totalKills = 0;
    double totalVPs = 0.0;
    if (sDatabase.RunQuery(totalRes,
        "SELECT COALESCE(SUM(kills), 0), COALESCE(SUM(victoryPoints), 0) FROM facWarStats"))
    {
        DBResultRow row;
        if (totalRes.GetRow(row)) {
            totalKills = row.GetUInt(0);
            totalVPs = row.GetDouble(1);
        }
    }

    PyDict* result = new PyDict();
        result->SetItemString("topKills", topKills);
        result->SetItemString("topVPs", topVPs);
        result->SetItemString("totalKills", new PyInt(totalKills));
        result->SetItemString("totalVPs", new PyFloat(totalVPs));
    return result;
}

PyResult FactionWarMgrService::GetStats_Character(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT kills, losses, victoryPoints FROM facWarStats WHERE characterID = %u", charID);
    DBResultRow row;
    if (!res.GetRow(row))
        return new PyDict();

    PyDict* dict = new PyDict();
        dict->SetItemString("kills", new PyInt(row.GetInt(0)));
        dict->SetItemString("losses", new PyInt(row.GetInt(1)));
        dict->SetItemString("victoryPoints", new PyFloat(row.GetDouble(2)));
    return dict;
}

PyResult FactionWarMgrService::GetStats_Corp(PyCallArgs &call) {
    // return dict of corpID → {corpID, kills, losses, victoryPoints, pilots}
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_Corp()");

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT c.corporationID,"
        "  COALESCE(SUM(s.kills), 0) AS kills,"
        "  COALESCE(SUM(s.losses), 0) AS losses,"
        "  COALESCE(SUM(s.victoryPoints), 0) AS vps,"
        "  COUNT(DISTINCT s.characterID) AS pilots"
        " FROM facWarStats s"
        " JOIN chrCharacters c ON s.characterID = c.characterID"
        " GROUP BY c.corporationID"))
        return new PyDict();

    PyDict* result = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 corpID = row.GetUInt(0);
        PyDict* info = new PyDict();
            info->SetItemString("corporationID", new PyInt(corpID));
            info->SetItemString("kills", new PyInt(row.GetUInt(1)));
            info->SetItemString("losses", new PyInt(row.GetUInt(2)));
            info->SetItemString("victoryPoints", new PyFloat(row.GetDouble(3)));
            info->SetItemString("pilots", new PyInt(row.GetUInt(4)));
        result->SetItem(new PyInt(corpID), info);
    }
    return result;
}

PyResult FactionWarMgrService::GetStats_Alliance(PyCallArgs &call) {
    // return dict of allianceID → {allianceID, kills, losses, victoryPoints, pilots}
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_Alliance()");

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT crp.allianceID,"
        "  COALESCE(SUM(s.kills), 0) AS kills,"
        "  COALESCE(SUM(s.losses), 0) AS losses,"
        "  COALESCE(SUM(s.victoryPoints), 0) AS vps,"
        "  COUNT(DISTINCT s.characterID) AS pilots"
        " FROM facWarStats s"
        " JOIN chrCharacters c ON s.characterID = c.characterID"
        " JOIN crpCorporations crp ON c.corporationID = crp.corporationID"
        " WHERE crp.allianceID > 0"
        " GROUP BY crp.allianceID"))
        return new PyDict();

    PyDict* result = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 allyID = row.GetUInt(0);
        PyDict* info = new PyDict();
            info->SetItemString("allianceID", new PyInt(allyID));
            info->SetItemString("kills", new PyInt(row.GetUInt(1)));
            info->SetItemString("losses", new PyInt(row.GetUInt(2)));
            info->SetItemString("victoryPoints", new PyFloat(row.GetDouble(3)));
            info->SetItemString("pilots", new PyInt(row.GetUInt(4)));
        result->SetItem(new PyInt(allyID), info);
    }
    return result;
}

PyResult FactionWarMgrService::GetStats_Militia(PyCallArgs &call) {
    // return dict of factionID → faction-level aggregate stats
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_Militia()");

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "  ff.factionID,"
        "  COALESCE((SELECT SUM(kills) FROM facWarStats WHERE factionID = ff.factionID), 0) AS kills,"
        "  COALESCE((SELECT SUM(losses) FROM facWarStats WHERE factionID = ff.factionID), 0) AS losses,"
        "  COALESCE((SELECT SUM(victoryPoints) FROM facWarStats WHERE factionID = ff.factionID), 0) AS vps,"
        "  (SELECT COUNT(*) FROM facWarCharacters WHERE factionID = ff.factionID) AS pilots,"
        "  (SELECT COUNT(*) FROM facWarSystems WHERE occupierID = ff.factionID) AS systems"
        " FROM facFactions ff"
        " WHERE ff.militiaCorporationID IS NOT NULL"))
        return new PyDict();

    PyDict* result = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 factionID = row.GetUInt(0);
        PyDict* info = new PyDict();
            info->SetItemString("factionID", new PyInt(factionID));
            info->SetItemString("kills", new PyInt(row.GetUInt(1)));
            info->SetItemString("losses", new PyInt(row.GetUInt(2)));
            info->SetItemString("victoryPoints", new PyFloat(row.GetDouble(3)));
            info->SetItemString("pilots", new PyInt(row.GetUInt(4)));
            info->SetItemString("systemsControlled", new PyInt(row.GetUInt(5)));
        result->SetItem(new PyInt(factionID), info);
    }
    return result;
}

PyResult FactionWarMgrService::GetStats_CorpPilots(PyCallArgs &call) {
    // return list of corp pilots in FW: [{characterID, characterName, factionID}]
    _log(FACWAR__CALL, "FacWarMgr::Handle_GetStats_CorpPilots()");

    uint32 corpID = call.client->GetCorporationID();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT c.characterID, c.characterName, fwc.factionID"
        " FROM facWarCharacters fwc"
        " JOIN chrCharacters c ON fwc.characterID = c.characterID"
        " WHERE c.corporationID = %u", corpID))
        return new PyList();

    PyList* result = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
            entry->SetItemString("characterID", new PyInt(row.GetUInt(0)));
            entry->SetItemString("characterName", new PyString(row.GetText(1)));
            entry->SetItemString("factionID", new PyInt(row.GetUInt(2)));
        result->AddItem(new PyObject("util.KeyVal", entry));
    }
    return result;
}

PyResult FactionWarMgrService::RefreshCorps(PyCallArgs &call) {
    // Force-clear GetFactionMilitiaCorporation cache so it reloads from DB
    _log(FACWAR__CALL, "FacWarMgr::Handle_RefreshCorps()");

    ObjectCachedMethodID method_id(GetName().c_str(), "GetWarFactions");
    this->m_cache->InvalidateCache(method_id);

    return PyStatic.NewNone();
}

