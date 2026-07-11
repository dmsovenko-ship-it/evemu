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
    Author:        Luck, caytchen
*/

#include "eve-server.h"


#include "mail/MailingListMgrService.h"

MailingListMgrService::MailingListMgrService() :
    Service("mailingListsMgr", eAccessLevel_Character)
{
    this->Add("GetJoinedLists", &MailingListMgrService::GetJoinedLists);
    this->Add("Create", &MailingListMgrService::Create);
    this->Add("Join", &MailingListMgrService::Join);
    this->Add("Leave", &MailingListMgrService::Leave);
    this->Add("Delete", &MailingListMgrService::Delete);
    this->Add("KickMembers", &MailingListMgrService::KickMembers);
    this->Add("GetMembers", &MailingListMgrService::GetMembers);
    this->Add("SetEntityAccess", &MailingListMgrService::SetEntityAccess);
    this->Add("ClearEntityAccess", &MailingListMgrService::ClearEntityAccess);
    this->Add("SetMembersMuted", &MailingListMgrService::SetMembersMuted);
    this->Add("SetMembersOperator", &MailingListMgrService::SetMembersOperator);
    this->Add("SetMembersClear", &MailingListMgrService::SetMembersClear);
    this->Add("SetDefaultAccess", &MailingListMgrService::SetDefaultAccess);
    this->Add("GetInfo", &MailingListMgrService::GetInfo);
    this->Add("GetSettings", &MailingListMgrService::GetSettings);
    this->Add("GetWelcomeMail", &MailingListMgrService::GetWelcomeMail);
    this->Add("SaveWelcomeMail", &MailingListMgrService::SaveWelcomeMail);
    this->Add("SendWelcomeMail", &MailingListMgrService::SendWelcomeMail);
    this->Add("ClearWelcomeMail", &MailingListMgrService::ClearWelcomeMail);
}

PyResult MailingListMgrService::GetJoinedLists(PyCallArgs& call)
{
    // @TODO: Test
    // no args
    sLog.Debug("MailingListMgrService", "Called GetJoinedLists stub" );

    return m_db.GetJoinedMailingLists(call.client->GetCharacterID());
}

PyResult MailingListMgrService::Create(PyCallArgs& call, PyWString* name, PyInt* defaultAccess, PyInt* defaultMemberAccess, std::optional<PyInt*> mailCost)
{
    // @TODO: Test
    sLog.Debug("MailingListMgrService", "Called Create stub" );
    uint32 r = m_db.CreateMailingList(call.client->GetCharacterID(), name->content(), defaultAccess->value(),
                                   defaultMemberAccess->value(), mailCost.has_value() ? mailCost.value()->value() : 0);
    if (r >= 0) {
        return new PyInt(r);
    }
    return nullptr;
}

PyResult MailingListMgrService::Join(PyCallArgs& call, PyRep* listName)
{
    std::string listNameStr = PyRep::StringContent(listName);

    // find list by name
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT id, displayName, defaultAccess, defaultMemberAccess FROM mailList WHERE displayName = '%s'",
        listNameStr.c_str());
    DBResultRow row;
    if (!res.GetRow(row))
        return PyStatic.NewNone();

    m_db.JoinMailingList(call.client->GetCharacterID(), listNameStr);

    // return mailing list object with .id for client
    PyDict* dict = new PyDict();
        dict->SetItemString("id", new PyInt(row.GetInt(0)));
        dict->SetItemString("displayName", new PyString(row.GetText(1)));
        dict->SetItemString("defaultAccess", new PyInt(row.GetInt(2)));
        dict->SetItemString("defaultMemberAccess", new PyInt(row.GetInt(3)));
    return new PyObject("util.KeyVal", dict);
}

PyResult MailingListMgrService::Leave(PyCallArgs& call, PyInt* listID)
{
    m_db.LeaveMailingList(call.client->GetCharacterID(), listID->value());
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::Delete(PyCallArgs& call, PyInt* listID)
{
    m_db.DeleteMailingList(call.client->GetCharacterID(), listID->value());
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::KickMembers(PyCallArgs& call, PyInt* listID, PyList* memberIDs)
{
    sLog.Debug("MailingListMgrService", "Called KickMembers" );

    std::vector<int32> ids;
    for (int i = 0; i < memberIDs->size(); i++) {
        PyRep* item = memberIDs->GetItem(i);
        if (item->IsInt() || item->IsLong())
            ids.push_back(PyRep::IntegerValueU32(item));
    }

    if (!ids.empty())
        m_db.KickMembers(listID->value(), ids);

    return PyStatic.NewNone();
}

PyResult MailingListMgrService::GetMembers(PyCallArgs& call, PyInt* listID)
{
    // @TODO: Stub
    sLog.Debug("MailingListMgrService", "Called GetMembers stub" );
    return m_db.GetMailingListMembers(listID->value());
}

PyResult MailingListMgrService::SetEntityAccess(PyCallArgs& call, PyInt* listID, PyInt* entityID, PyInt* access)
{
    sLog.Debug("MailingListMgrService", "Called SetEntityAccess" );
    m_db.MailingListSetEntityAccess(entityID->value(), access->value(), listID->value());
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::ClearEntityAccess(PyCallArgs& call, PyInt* listID, PyInt* entityID)
{
    sLog.Debug("MailingListMgrService", "Called ClearEntityAccess" );
    m_db.MailingListClearEntityAccess(entityID->value(), listID->value());
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::SetMembersMuted(PyCallArgs& call, PyInt* listID, PyList* memberIDs)
{
    sLog.Debug("MailingListMgrService", "Called SetMembersMuted" );
    std::vector<int32> ids;
    for (int i = 0; i < memberIDs->size(); i++) {
        PyRep* item = memberIDs->GetItem(i);
        if (item->IsInt() || item->IsLong())
            ids.push_back(PyRep::IntegerValueU32(item));
    }
    if (!ids.empty())
        m_db.SetMembersRole(listID->value(), ids, mailingListMemberMuted);
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::SetMembersOperator(PyCallArgs& call, PyInt* listID, PyList* memberIDs)
{
    sLog.Debug("MailingListMgrService", "Called SetMembersOperator" );
    std::vector<int32> ids;
    for (int i = 0; i < memberIDs->size(); i++) {
        PyRep* item = memberIDs->GetItem(i);
        if (item->IsInt() || item->IsLong())
            ids.push_back(PyRep::IntegerValueU32(item));
    }
    if (!ids.empty())
        m_db.SetMembersRole(listID->value(), ids, mailingListMemberOperator);
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::SetMembersClear(PyCallArgs& call, PyInt* listID, PyList* memberIDs)
{
    sLog.Debug("MailingListMgrService", "Called SetMembersClear" );
    std::vector<int32> ids;
    for (int i = 0; i < memberIDs->size(); i++) {
        PyRep* item = memberIDs->GetItem(i);
        if (item->IsInt() || item->IsLong())
            ids.push_back(PyRep::IntegerValueU32(item));
    }
    if (!ids.empty())
        m_db.SetMembersRole(listID->value(), ids, mailingListMemberDefault);
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::SetDefaultAccess(PyCallArgs& call, PyInt* listID, PyInt* defaultAccess, PyInt* defaultMemberAccess, std::optional<PyInt*> mailCost)
{
    sLog.Debug("MailingListMgrService", "Called SetDefaultAccess" );
    m_db.SetMailingListDefaultAccess(listID->value(), defaultAccess->value(),
                                      defaultMemberAccess->value(), mailCost.has_value() ? mailCost.value()->value() : 0);
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::GetInfo(PyCallArgs& call, PyInt* listID)
{
    sLog.Debug("MailingListMgrService", "Called GetInfo" );
    PyObject* info = m_db.MailingListGetInfo(listID->value());
    if (info == nullptr)
        return PyStatic.NewNone();
    return info;
}

PyResult MailingListMgrService::GetSettings(PyCallArgs& call, PyInt* listID)
{
    // @TODO: Test
    sLog.Debug("MailingListMgrService", "Called GetSettings stub" );
    // return:
    // .access: list (ownerID, accessLevel)
    // .defaultAccess
    // .defaultMemberAccess

    return m_db.MailingListGetSettings(listID->value());
}

PyResult MailingListMgrService::GetWelcomeMail(PyCallArgs& call, PyInt* listID)
{
    sLog.Debug("MailingListMgrService", "Called GetWelcomeMail" );
    PyString* mail = m_db.MailingListGetWelcomeMail(listID->value());
    if (mail == nullptr)
        return PyStatic.NewNone();
    return mail;
}

PyResult MailingListMgrService::SaveWelcomeMail(PyCallArgs& call, PyInt* listID, PyWString* title, PyWString* body)
{
    sLog.Debug("MailingListMgrService", "Called SaveWelcomeMail" );
    m_db.MailingListSaveWelcomeMail(listID->value(), title->content(), body->content());
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::SendWelcomeMail(PyCallArgs& call, PyInt* listID, PyWString* title, PyWString* body)
{
    sLog.Debug("MailingListMgrService", "Called SendWelcomeMail" );
    // Save then send to all current members
    m_db.MailingListSaveWelcomeMail(listID->value(), title->content(), body->content());

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT characterID FROM mailListUsers WHERE listID = %u",
        listID->value());

    std::vector<int> recipients;
    DBResultRow row;
    while (res.GetRow(row))
        recipients.push_back(row.GetInt(0));

    if (!recipients.empty())
        m_db.SendMail(call.client->GetCharacterID(), recipients, 0, 0,
                      title->content(), body->content(), 0, 0);
    return PyStatic.NewNone();
}

PyResult MailingListMgrService::ClearWelcomeMail(PyCallArgs& call, PyInt* listID)
{
    sLog.Debug("MailingListMgrService", "Called ClearWelcomeMail" );
    m_db.MailingListClearWelcomeMail(listID->value());
    return PyStatic.NewNone();
}
