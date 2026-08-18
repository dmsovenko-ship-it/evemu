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
    Author:        Reve
*/

//work in progress

#include "eve-server.h"

#include "Client.h"
#include "dungeon/DungeonExplorationMgrService.h"
#include "expedition/ExpeditionMgr.h"

DungeonExplorationMgrService::DungeonExplorationMgrService() :
    Service("dungeonExplorationMgr")
{
    this->Add("GetMyEscalatingPathDetails", &DungeonExplorationMgrService::GetMyEscalatingPathDetails);
    this->Add("DeleteExpiredPathStep", &DungeonExplorationMgrService::DeleteExpiredPathStep);
}

PyResult DungeonExplorationMgrService::GetMyEscalatingPathDetails(PyCallArgs &call) {
    // cached response — the Journal "Expeditions" tab. Returns a util.Rowset
    // whose rows have: instanceID, dungeon, destDungeon, pathStep, expiryTime.
    // `dungeon`/`pathStep` are util.KeyVal objects (the client reads
    // dungeon.solarSystemID / dungeon.creationTime / pathStep.journalEntryTemplateID).
    _log(DUNG__MESSAGE, "DungeonExplorationMgrService::Handle_GetMyEscalatingPathDetails() size=%lu", call.tuple->size());

    uint32 charID = call.client->GetCharacterID();
    ExpeditionMgr::ExpeditionView exp;
    if (!sExpMgr.GetExpedition(charID, exp)) {
        util_Rowset empty;
        empty.header.push_back("instanceID");
        empty.header.push_back("dungeon");
        empty.header.push_back("destDungeon");
        empty.header.push_back("pathStep");
        empty.header.push_back("expiryTime");
        return empty.Encode();
    }

    // dungeon (source) — where the next site is
    PyDict* dungeonDict = new PyDict();
        dungeonDict->SetItemString("instanceID",       new PyLong(exp.instanceID));
        dungeonDict->SetItemString("solarSystemID",    new PyInt(exp.solarSystemID));
        dungeonDict->SetItemString("creationTime",     new PyLong(exp.creationTime));
        dungeonDict->SetItemString("dungeonNameID",    new PyInt(280001));  // generic expedition name
    // destDungeon — the site the player actually runs (same as dungeon here)
    PyDict* destDungeonDict = new PyDict();
        destDungeonDict->SetItemString("instanceID",   new PyLong(exp.instanceID));
        destDungeonDict->SetItemString("solarSystemID", new PyInt(exp.solarSystemID));
        destDungeonDict->SetItemString("creationTime", new PyLong(exp.creationTime));
        destDungeonDict->SetItemString("dungeonNameID", new PyInt(280001));
    // pathStep — description shown in the journal (message text by templateID)
    PyDict* pathDict = new PyDict();
        pathDict->SetItemString("journalEntryTemplateID", new PyInt(280003));  // generic "expedition site"
        pathDict->SetItemString("instanceID", new PyLong(exp.instanceID));

    util_Rowset rs;
    rs.header.push_back("instanceID");
    rs.header.push_back("dungeon");
    rs.header.push_back("destDungeon");
    rs.header.push_back("pathStep");
    rs.header.push_back("expiryTime");

    PyList* fieldData = new PyList();
        fieldData->AddItem(new PyLong(exp.instanceID));
        fieldData->AddItem(new PyObject("util.KeyVal", dungeonDict));
        fieldData->AddItem(new PyObject("util.KeyVal", destDungeonDict));
        fieldData->AddItem(new PyObject("util.KeyVal", pathDict));
        fieldData->AddItem(new PyLong(exp.expiryTime));
    rs.lines->AddItem(fieldData);

    return rs.Encode();
}

PyResult DungeonExplorationMgrService::DeleteExpiredPathStep(PyCallArgs& call, PyInt* instanceID) {
    _log(DUNG__MESSAGE, "DungeonExplorationMgrService::Handle_DeleteExpiredPathStep() size=%lu", call.tuple->size());
    // Client removes an expired expedition entry — nothing persistent to delete.
    return PyStatic.NewNone();
}
