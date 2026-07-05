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
    Author:        Zhur, Allan
*/

#include "eve-server.h"


#include "account/TutorialService.h"

TutorialService::TutorialService() :
    Service("tutorialSvc")
{
    this->Add("GetTutorials", &TutorialService::GetTutorials);
    this->Add("GetTutorialInfo", &TutorialService::GetTutorialInfo);
    this->Add("GetTutorialAgents", &TutorialService::GetTutorialAgents);
    this->Add("GetCriterias", &TutorialService::GetCriterias);
    this->Add("GetCategories", &TutorialService::GetCategories);
    this->Add("GetCharacterTutorialState", &TutorialService::GetCharacterTutorialState);
    this->Add("GetTutorialsAndConnections", &TutorialService::GetTutorialsAndConnections);
    this->Add("GetCareerAgents", &TutorialService::GetCareerAgents);
    this->Add("LogStarted", &TutorialService::LogStarted);
    this->Add("LogCompleted", &TutorialService::LogCompleted);
    this->Add("LogAborted", &TutorialService::LogAborted);
}

PyResult TutorialService::GetTutorials(PyCallArgs &call) {
  sLog.Warning( "TutorialService::Handle_GetTutorials()", "size=%lu", call.tuple->size());
  call.Dump(SERVICE__CALL_DUMP);
    return(m_db.GetAllTutorials());
}

PyResult TutorialService::GetTutorialInfo(PyCallArgs &call, PyInt* tutorialID) {
  sLog.Warning( "TutorialService::Handle_GetTutorialInfo()", "size=%lu", call.tuple->size());
  call.Dump(SERVICE__CALL_DUMP);
    Rsp_GetTutorialInfo rsp;

    rsp.pagecriterias = m_db.GetPageCriterias(tutorialID->value());
    if (rsp.pagecriterias == NULL) {
        codelog(SERVICE__ERROR, "An error occured while getting pagecriterias for tutorial %u.", tutorialID->value());
        return nullptr;
    }

    rsp.pages = m_db.GetPages(tutorialID->value());
    if (rsp.pages == NULL) {
        codelog(SERVICE__ERROR, "An error occured while getting pages for tutorial %u.", tutorialID->value());
        return nullptr;
    }

    rsp.tutorial = m_db.GetTutorial(tutorialID->value());
    if (rsp.tutorial == NULL) {
        codelog(SERVICE__ERROR, "An error occured while getting tutorial %u.", tutorialID->value());
        return nullptr;
    }

    rsp.criterias = m_db.GetTutorialCriterias(tutorialID->value());
    if (rsp.criterias == NULL) {
        codelog(SERVICE__ERROR, "An error occured while getting criterias for tutorial %u.", tutorialID->value());
        return nullptr;
    }

    return(rsp.Encode());
}

PyResult TutorialService::GetTutorialAgents(PyCallArgs &call, PyList* agentIDs) {
    /*  this should be cached
          [PyTuple 4 items]
            [PyInt 1]
            [PyString "GetTutorialAgents"]
            [PyTuple 1 items]
              [PyList 12 items]
                [PyInt 3018921]
                [PyInt 3019349]
                [PyInt 3019337]
                [PyInt 3018935]
                [PyInt 3019371]
                [PyInt 3019355]
                [PyInt 3018923]
                [PyInt 3019341]
                [PyInt 3019333]
                [PyInt 3018920]
                [PyInt 3019346]
                [PyInt 3019343]
            [PyDict 1 kvp]
              [PyString "machoVersion"]
              [PyInt 1]

      [PySubStream 377 bytes]
        [PyList 12 items]
          [PyPackedRow 28 bytes]
            ["agentID" => <3018920> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015037> [I4]]
            ["bloodlineID" => <14> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <0> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3018921> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015021> [I4]]
            ["bloodlineID" => <6> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3018923> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015029> [I4]]
            ["bloodlineID" => <12> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3018935> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015027> [I4]]
            ["bloodlineID" => <11> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019333> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015036> [I4]]
            ["bloodlineID" => <7> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019337> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015010> [I4]]
            ["bloodlineID" => <5> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019341> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015016> [I4]]
            ["bloodlineID" => <8> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019343> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015041> [I4]]
            ["bloodlineID" => <3> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019346> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015046> [I4]]
            ["bloodlineID" => <4> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019349> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015020> [I4]]
            ["bloodlineID" => <13> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <0> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019355> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015005> [I4]]
            ["bloodlineID" => <11> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <0> [Bool]]
          [PyPackedRow 28 bytes]
            ["agentID" => <3019371> [I4]]
            ["agentTypeID" => <8> [I4]]
            ["divisionID" => <22> [I4]]
            ["level" => <1> [UI1]]
            ["stationID" => <60015001> [I4]]
            ["bloodlineID" => <1> [UI1]]
            ["quality" => <0> [I4]]
            ["corporationID" => <0> [I4]]
            ["gender" => <1> [Bool]]
                */
    sLog.White("TutorialService::GetTutorialAgents()", "size=%lu", call.tuple->size());

    // Build list of agent IDs from the call argument
    std::vector<uint32> agentIDs;
    for (size_t i = 0; i < agentIDs->size(); ++i) {
        PyRep* item = agentIDs->GetItem(i);
        if (item->IsInt())
            agentIDs.push_back(item->AsInt()->value());
    }

    if (agentIDs.empty())
        return new PyList();

    // Query agtAgents for the requested agent IDs
    DBQueryResult res;
    std::string idList;
    for (size_t i = 0; i < agentIDs.size(); ++i) {
        if (i > 0) idList += ",";
        idList += std::to_string(agentIDs[i]);
    }
    sDatabase.RunQuery(res,
        "SELECT agt.agentID, agt.agentTypeID, agt.divisionID, agt.level, "
        "  agt.locationID AS stationID, chr.bloodlineID, agt.quality, "
        "  agt.corporationID, chr.gender"
        " FROM agtAgents AS agt"
        " LEFT JOIN chrNPCCharacters AS chr ON chr.characterID = agt.agentID"
        " WHERE agt.agentID IN (%s)", idList.c_str());

    // Build DBRowDescriptor matching the client's expected columns
    DBRowDescriptor* header = new DBRowDescriptor();
    header->AddColumn("agentID",       DBTYPE_I4);
    header->AddColumn("agentTypeID",   DBTYPE_I4);
    header->AddColumn("divisionID",    DBTYPE_I4);
    header->AddColumn("level",         DBTYPE_UI1);
    header->AddColumn("stationID",     DBTYPE_I4);
    header->AddColumn("bloodlineID",   DBTYPE_UI1);
    header->AddColumn("quality",       DBTYPE_I4);
    header->AddColumn("corporationID", DBTYPE_I4);
    header->AddColumn("gender",        DBTYPE_BOOL);

    DBResultRow row;
    PyList* result = new PyList();
    while (res.GetRow(row)) {
        PyPackedRow* packed = new PyPackedRow(header);
        packed->SetField("agentID",       new PyInt(row.GetUInt(0)));
        packed->SetField("agentTypeID",   new PyInt(row.GetUInt(1)));
        packed->SetField("divisionID",    new PyInt(row.GetUInt(2)));
        packed->SetField("level",         new PyInt(row.GetUInt(3)));
        packed->SetField("stationID",     new PyInt(row.GetUInt(4)));
        packed->SetField("bloodlineID",   new PyInt(row.GetUInt(5)));
        packed->SetField("quality",       new PyInt(row.GetInt(6)));
        packed->SetField("corporationID", new PyInt(row.GetUInt(7)));
        packed->SetField("gender",        row.GetUInt(8) ? PyStatic.NewTrue() : PyStatic.NewFalse());
        result->AddItem(packed);
    }
    return result;
}

PyResult TutorialService::GetCriterias(PyCallArgs &call) {
  sLog.White( "TutorialService::Handle_GetCriterias()", "size=%lu", call.tuple->size());
  call.Dump(SERVICE__CALL_DUMP);
    return(m_db.GetAllCriterias());
}

PyResult TutorialService::GetCategories(PyCallArgs &call) {
  sLog.White( "TutorialService::Handle_GetCategories()", "size=%lu", call.tuple->size());
  call.Dump(SERVICE__CALL_DUMP);
    return(m_db.GetCategories());
}

//00:25:53 L TutorialService::Handle_GetCharacterTutorialState(): size= 0
PyResult TutorialService::GetCharacterTutorialState(PyCallArgs& call) {
  /*  Empty Call  */

    return new PyInt( 0 );
}

PyResult TutorialService::GetTutorialsAndConnections(PyCallArgs& call) {
    /*  no logs */
  /*  This is used to link tutorials using connections to other tutorials  */
            /*
            t, tc = sm.RemoteSvc('tutorialSvc').GetTutorialsAndConnections()
            self.tutorials = t.Index('tutorialID')
            tc = tc.Filter('tutorialID')
            self.tutorialConnections = defaultdict(dict)
            for tutID, rows in tc.iteritems():
                for each in rows:
                    self.tutorialConnections[tutID][each.raceID] = each.nextTutorialID
    */

            /*  FIXME  this needs work.  not sure what's wrong, but i DO know our db is incomplete
    uint8 raceID = call.client->GetChar()->race();
    return (m_db.GetTutorialsAndConnections(raceID));
    */
    return PyStatic.NewNone();
}

PyResult TutorialService::LogStarted(PyCallArgs& call, PyInt* tutorialID, PyInt* pageNo, PyInt* time) {
    sLog.White("TutorialService::LogStarted()", "tutorialID=%u pageNo=%u", tutorialID->value(), pageNo->value());
    return PyStatic.NewNone();
}

PyResult TutorialService::LogCompleted(PyCallArgs& call, PyInt* tutorialID, PyInt* pageNo, PyInt* time) {
    sLog.White("TutorialService::LogCompleted()", "tutorialID=%u pageNo=%u", tutorialID->value(), pageNo->value());
    return PyStatic.NewNone();
}

PyResult TutorialService::LogAborted(PyCallArgs& call, PyInt* tutorialID, PyInt* pageNo, PyInt* time) {
    sLog.White("TutorialService::LogAborted()", "tutorialID=%u pageNo=%u", tutorialID->value(), pageNo->value());
    return PyStatic.NewNone();
}

PyResult TutorialService::GetCareerAgents(PyCallArgs& call) {
    sLog.White("TutorialService::GetCareerAgents()", "");
    PyRep* mapping = m_db.GetCareerAgentMapping();
    if (mapping == nullptr)
        return PyStatic.NewNone();
    return mapping;
}


/**
            sm.RemoteSvc('tutorialSvc').LogCompleted(tutorialID, pageNo, int(time))
        elif status == 'aborted':
            stat[sequenceID] = 'done'
            sm.RemoteSvc('tutorialSvc').LogAborted(tutorialID, pageNo, int(time))

                categories = sm.RemoteSvc('tutorialSvc').GetCategories()
                for category in categories:
                    self.categories[category.categoryID] = category
                    self.categories[category.categoryID].categoryName = localization.GetByMessageID(category.categoryNameID)
                    self.categories[category.categoryID].description = localization.GetByMessageID(category.descriptionID)

                criterias = sm.RemoteSvc('tutorialSvc').GetCriterias()
                for criteria in criterias:
                    self.criterias[criteria.criteriaID] = criteria
            actions = sm.RemoteSvc('tutorialSvc').GetActions()
            for action in actions:
                self.actions[action.actionID] = action

            tutData = sm.RemoteSvc('tutorialSvc').GetTutorialInfo(tutorialID)
                sm.RemoteSvc('tutorialSvc').LogAppClosed(tutorialID, pageNo, int(time))
                        sm.RemoteSvc('tutorialSvc').LogClosed(tutorialID, pageNo, int(time))
                    sm.RemoteSvc('tutorialSvc').LogStarted(tutorialID, pageNo, int(time))
            sm.RemoteSvc('tutorialSvc').LogCompleted(tutorialID, pageNo, int(time))
        return sm.RemoteSvc('tutorialLocationSvc').GiveTutorialGoodies(tutorialID, pageID, pageNo)
                tutData = sm.RemoteSvc('tutorialSvc').GetTutorialInfo(VID)

        rs = sm.RemoteSvc('tutorialSvc').GetCharacterTutorialState()
        if not rs or len(rs) == 0:
            return



            */
