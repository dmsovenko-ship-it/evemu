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


#include "EntityList.h"
#include "chat/OnlineStatusService.h"

OnlineStatusService::OnlineStatusService() :
    Service("onlineStatus", eAccessLevel_Character)
{
    this->Add("GetInitialState", &OnlineStatusService::GetInitialState);
    this->Add("GetOnlineStatus", &OnlineStatusService::GetOnlineStatus);
}

PyResult OnlineStatusService::GetInitialState(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();

    DBRowDescriptor *header = new DBRowDescriptor();
    header->AddColumn("contactID", DBTYPE_I4);
    header->AddColumn("online", DBTYPE_BOOL);
    CRowSet *rowset = new CRowSet(&header);

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT contactID FROM chrContacts WHERE characterID = %u AND inContacts = 1",
        charID);

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 contactID = row.GetUInt(0);
        PyPackedRow* pRow = rowset->NewRow();
        pRow->SetField("contactID", new PyInt(contactID));
        pRow->SetField("online", sEntityList.PyIsOnline(contactID));
    }
    return rowset;
}

PyResult OnlineStatusService::GetOnlineStatus(PyCallArgs &call, PyInt* characterID) {
    // this is used to query the online state of a character by charID.
     return sEntityList.PyIsOnline(characterID->value());
}

/*
==================== Sent from Server 81 bytes

[PyObjectData Name: macho.Notification]
  [PyTuple 7 items]
    [PyInt 12]
    [PyObjectData Name: macho.MachoAddress]
      [PyTuple 4 items]
        [PyInt 1]
        [PyInt 806438]
        [PyNone]
        [PyNone]
    [PyObjectData Name: macho.MachoAddress]
      [PyTuple 4 items]
        [PyInt 4]
        [PyString "OnContactLoggedOn"]
        [PyList 0 items]
        [PyString "clientID"]
    [PyInt 5654387]
    [PyTuple 1 items]
      [PyTuple 2 items]
        [PyInt 0]
        [PySubStream 15 bytes]
          [PyTuple 2 items]
            [PyInt 0]
            [PyTuple 2 items]
              [PyInt 1]
              [PyTuple 1 items]
                [PyInt 649670823]
    [PyNone]
    [PyNone]

==================== Sent from Server 81 bytes

[PyObjectData Name: macho.Notification]
  [PyTuple 6 items]
    [PyInt 12]
    [PyObjectData Name: macho.MachoAddress]
      [PyTuple 4 items]
        [PyInt 1]
        [PyInt 698462]
        [PyNone]
        [PyNone]
    [PyObjectData Name: macho.MachoAddress]
      [PyTuple 4 items]
        [PyInt 4]
        [PyString "OnContactLoggedOff"]
        [PyList 0 items]
        [PyString "clientID"]
    [PyInt 5654387]
    [PyTuple 1 items]
      [PyTuple 2 items]
        [PyInt 0]
        [PySubStream 15 bytes]
          [PyTuple 2 items]
            [PyInt 0]
            [PyTuple 2 items]
              [PyInt 1]
              [PyTuple 1 items]
                [PyInt 1610990724]
    [PyNone]


*/