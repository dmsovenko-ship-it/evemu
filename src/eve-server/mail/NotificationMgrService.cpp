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
    Author:     caytchen
*/

#include "eve-server.h"

#include "Client.h"
#include "mail/NotificationMgrService.h"

// this service is part of mail and used with the 'notifications' tab of mail window

NotificationMgrService::NotificationMgrService() :
    Service("notificationMgr", eAccessLevel_Character)
{
    this->Add("GetByGroupID", &NotificationMgrService::GetByGroupID);
    this->Add("GetUnprocessed", &NotificationMgrService::GetUnprocessed);
    this->Add("MarkGroupAsProcessed", &NotificationMgrService::MarkGroupAsProcessed);
    this->Add("MarkAllAsProcessed", &NotificationMgrService::MarkAllAsProcessed);
    this->Add("MarkAsProcessed", &NotificationMgrService::MarkAsProcessed);
    this->Add("DeleteGroupNotifications", &NotificationMgrService::DeleteGroupNotifications);
    this->Add("DeleteAllNotifications", &NotificationMgrService::DeleteAllNotifications);
    this->Add("DeleteNotifications", &NotificationMgrService::DeleteNotifications);
}

PyResult NotificationMgrService::GetByGroupID(PyCallArgs &call, PyInt* groupID)
{
    uint32 charID = call.client->GetCharacterID();

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT n.notificationID, n.typeID, n.senderID, n.receiverID,"
        "       n.processed, n.created, t.data, n.deleted"
        " FROM notification n"
        " LEFT JOIN notificationText t ON t.notificationID = n.notificationID"
        " WHERE n.receiverID = %u AND n.deleted = 0"
        " ORDER BY n.created DESC"
        " LIMIT 200",
        charID);

    return DBResultToCRowset(res);
}

PyResult NotificationMgrService::GetUnprocessed(PyCallArgs &call)
{
    uint32 charID = call.client->GetCharacterID();

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT n.notificationID, n.typeID, n.senderID, n.receiverID,"
        "       n.processed, n.created, t.data, n.deleted"
        " FROM notification n"
        " LEFT JOIN notificationText t ON t.notificationID = n.notificationID"
        " WHERE n.receiverID = %u AND n.processed = 0 AND n.deleted = 0"
        " ORDER BY n.created DESC"
        " LIMIT 100",
        charID);

    return DBResultToCRowset(res);
}

PyResult NotificationMgrService::MarkGroupAsProcessed(PyCallArgs &call, PyInt* groupID)
{
    uint32 charID = call.client->GetCharacterID();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET processed = 1"
        " WHERE receiverID = %u AND deleted = 0",
        charID);
    return PyStatic.NewNone();
}

PyResult NotificationMgrService::MarkAllAsProcessed(PyCallArgs &call)
{
    uint32 charID = call.client->GetCharacterID();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET processed = 1"
        " WHERE receiverID = %u AND deleted = 0",
        charID);
    return PyStatic.NewNone();
}

PyResult NotificationMgrService::MarkAsProcessed(PyCallArgs &call, PyList* notificationIDsToMarkAsRead)
{
    uint32 charID = call.client->GetCharacterID();
    if (notificationIDsToMarkAsRead == nullptr or notificationIDsToMarkAsRead->size() == 0)
        return PyStatic.NewNone();

    // build comma-separated list of IDs
    std::string ids;
    PyList::const_iterator itr = notificationIDsToMarkAsRead->begin();
    while (itr != notificationIDsToMarkAsRead->end()) {
        if (!ids.empty()) ids += ",";
        ids += std::to_string(PyRep::IntegerValueU32(*itr));
        ++itr;
    }

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET processed = 1"
        " WHERE notificationID IN (%s) AND receiverID = %u",
        ids.c_str(), charID);
    return PyStatic.NewNone();
}

PyResult NotificationMgrService::DeleteGroupNotifications(PyCallArgs &call, PyInt* groupID)
{
    uint32 charID = call.client->GetCharacterID();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET deleted = 1"
        " WHERE receiverID = %u AND deleted = 0",
        charID);
    return PyStatic.NewNone();
}

PyResult NotificationMgrService::DeleteAllNotifications(PyCallArgs &call)
{
    uint32 charID = call.client->GetCharacterID();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET deleted = 1"
        " WHERE receiverID = %u AND deleted = 0",
        charID);
    return PyStatic.NewNone();
}

PyResult NotificationMgrService::DeleteNotifications(PyCallArgs &call, PyList* notificatinIDs)
{
    uint32 charID = call.client->GetCharacterID();
    if (notificatinIDs == nullptr or notificatinIDs->size() == 0)
        return PyStatic.NewNone();

    std::string ids;
    PyList::const_iterator itr = notificatinIDs->begin();
    while (itr != notificatinIDs->end()) {
        if (!ids.empty()) ids += ",";
        ids += std::to_string(PyRep::IntegerValueU32(*itr));
        ++itr;
    }

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE notification SET deleted = 1"
        " WHERE notificationID IN (%s) AND receiverID = %u",
        ids.c_str(), charID);
    return PyStatic.NewNone();
}
