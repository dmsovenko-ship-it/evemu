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

#include "account/TutorialDB.h"

PyRep *TutorialDB::GetPageCriterias(uint32 tutorialID) {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT pageID, criteriaID"
        " FROM tutorial_pages"
        " JOIN tutorial_page_criteria USING (pageID)"
        " WHERE tutorialID=%u", tutorialID))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetPages(uint32 tutorialID) {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT pageID, pageNumber, pageName, text AS textID, imagePath, audioPath,"
        " 0 AS uiPointerID, 0 AS uiPointerTextID, 0 AS pageActionID, 0 AS dataID"
        " FROM tutorial_pages"
        " WHERE tutorialID=%u"
        " ORDER BY pageNumber", tutorialID))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetTutorial(uint32 tutorialID) {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT tutorialID, tutorialName AS tutorialNameID, nextTutorialID, 0 AS dataID"
        " FROM tutorials"
        " WHERE tutorialID=%u", tutorialID))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetTutorialCriterias(uint32 tutorialID) {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT criteriaID"
        " FROM tutorials_criterias"
        " WHERE tutorialID=%u", tutorialID))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetAllTutorials() {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT tutorialID, tutorialName AS tutorialNameID, nextTutorialID, categoryID, 0 AS dataID"
        " FROM tutorials"))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetAllCriterias() {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT criteriaID, criteriaName, messageText AS messageTextID, audioPath, 0 AS dataID"
        " FROM tutorial_criteria"))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetCategories() {
    DBQueryResult res;

    if (!sDatabase.RunQuery(res,
        "SELECT categoryID, categoryName AS categoryNameID, description AS descriptionID, 0 AS dataID"
        " FROM tutorial_categories"))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

PyRep *TutorialDB::GetCareerAgentMapping() {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT careerType, agentID"
        " FROM tutorial_career_agents"))
    {
        codelog(DATABASE__ERROR, "Error in GetCareerAgentMapping query: %s", res.error.c_str());
        return nullptr;
    }
    // Group agents by careerType into a dict
    PyDict* result = new PyDict();
    DBResultRow row;
    std::map<std::string, PyList*> groups;
    while (res.GetRow(row)) {
        std::string careerType = row.GetText(0);
        uint32 agentID = row.GetUInt(1);
        auto it = groups.find(careerType);
        if (it == groups.end()) {
            PyList* lst = new PyList();
            lst->AddItem(new PyInt(agentID));
            groups.emplace(careerType, lst);
        } else {
            it->second->AddItem(new PyInt(agentID));
        }
    }
    for (auto& pair : groups)
        result->SetItemString(pair.first.c_str(), pair.second);
    return result;
}

PyRep *TutorialDB::GetTutorialsAndConnections(uint8 raceID) {
    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT tutorialID, %u AS raceID, nextTutorialID FROM tutorials", raceID);
    return DBResultToRowset(res);
}
