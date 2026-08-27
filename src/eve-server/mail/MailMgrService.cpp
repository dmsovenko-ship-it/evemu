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

#include <set>

#include "eve-server.h"


#include "mail/MailMgrService.h"
#include "EVE_Mail.h"

MailMgrService::MailMgrService() :
    Service("mailMgr", eAccessLevel_Character)
{
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyWString*, PyBool*, PyBool*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyString*, PyBool*, PyBool*)>(&MailMgrService::SendMail));
    // Client sends isReplyTo/isForwardedFrom as mixed PyInt*/PyBool*
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyWString*, PyInt*, PyBool*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyString*, PyInt*, PyBool*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyWString*, PyBool*, PyInt*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyString*, PyBool*, PyInt*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyWString*, PyInt*, PyInt*)>(&MailMgrService::SendMail));
    this->Add("SendMail", static_cast<PyResult(MailMgrService::*)(PyCallArgs&, PyList*, std::optional<PyInt*>, std::optional<PyInt*>, PyWString*, PyString*, PyInt*, PyInt*)>(&MailMgrService::SendMail));
    this->Add("PrimeOwners", &MailMgrService::PrimeOwners);
    this->Add("SyncMail", &MailMgrService::SyncMail);
    this->Add("GetMailHeaders", &MailMgrService::GetMailHeaders);
    this->Add("MoveToTrash", &MailMgrService::MoveToTrash);
    this->Add("MoveFromTrash", &MailMgrService::MoveFromTrash);
    this->Add("MarkAsUnread", &MailMgrService::MarkAsUnread);
    this->Add("MarkAsRead", &MailMgrService::MarkAsRead);
    this->Add("MoveAllToTrash", &MailMgrService::MoveAllToTrash);
    this->Add("MoveToTrashByLabel", &MailMgrService::MoveToTrashByLabel);
    this->Add("MoveToTrashByList", &MailMgrService::MoveToTrashByList);
    this->Add("MarkAllAsUnread", &MailMgrService::MarkAllAsUnread);
    this->Add("MarkAsUnreadByLabel", &MailMgrService::MarkAsUnreadByLabel);
    this->Add("MarkAsUnreadByList", &MailMgrService::MarkAsUnreadByList);
    this->Add("MarkAllAsRead", &MailMgrService::MarkAllAsRead);
    this->Add("MarkAsReadByLabel", &MailMgrService::MarkAsReadByLabel);
    this->Add("MarkAsReadByList", &MailMgrService::MarkAsReadByList);
    this->Add("MoveAllFromTrash", &MailMgrService::MoveAllFromTrash);
    this->Add("EmptyTrash", &MailMgrService::EmptyTrash);
    this->Add("DeleteMail", &MailMgrService::DeleteMail);
    this->Add("GetBody", &MailMgrService::GetBody);
    this->Add("AssignLabels", &MailMgrService::AssignLabels);
    this->Add("RemoveLabels", &MailMgrService::RemoveLabels);

    // implemented
    this->Add("GetLabels", &MailMgrService::GetLabels);
    this->Add("EditLabel", &MailMgrService::EditLabel);
    this->Add("CreateLabel", &MailMgrService::CreateLabel);
    this->Add("DeleteLabel", &MailMgrService::DeleteLabel);
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyWString* body, PyBool* isReplyTo, PyBool* isForwardedFrom)
{
    std::vector<int32> characters;

    PyList::const_iterator list_2_cur = toCharacterIDs->begin();
    for (size_t list_2_index(0); list_2_cur != toCharacterIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_SendMail failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        characters.push_back(t->value());
    }

    int sender = call.client->GetCharacterID();

    // rate limit: max 5 messages per minute
    DBQueryResult rateRes;
    sDatabase.RunQuery(rateRes,
        "SELECT COUNT(*) FROM mailMessage"
        " WHERE senderID = %u AND sentDate > %lli",
        sender, (int64)(GetFileTimeNow() - EvE::Time::Minute));
    DBResultRow rateRow;
    if (rateRes.GetRow(rateRow) and rateRow.GetInt(0) >= mailMaxMessagePerMinute) {
        call.client->SendErrorMsg("You have reached the maximum number of messages per minute.");
        return new PyInt(0);
    }

    // recipient limit
    size_t totalRecipients = characters.size();
    if (listID.has_value()) {
        DBQueryResult listRes;
        sDatabase.RunQuery(listRes, "SELECT COUNT(*) FROM mailListUsers WHERE listID = %u", listID.value()->value());
        DBResultRow listRow;
        if (listRes.GetRow(listRow))
            totalRecipients += listRow.GetInt(0);
    }
    if (totalRecipients > mailMaxRecipients) {
        call.client->SendErrorMsg("Too many recipients. Maximum is %u.", mailMaxRecipients);
        return new PyInt(0);
    }

    // blocked-contacts filter: remove recipients who have blocked the sender
    if (!characters.empty()) {
        std::string blockedSQL = "SELECT contactID FROM chrContacts WHERE blocked = 1 AND ownerID IN (";
        for (size_t i = 0; i < characters.size(); ++i) {
            if (i > 0) blockedSQL += ",";
            blockedSQL += std::to_string(characters[i]);
        }
        blockedSQL += ")";
        DBQueryResult blockedRes;
        sDatabase.RunQuery(blockedRes, blockedSQL.c_str());

        std::set<uint32> blockedIDs;
        DBResultRow blockedRow;
        while (blockedRes.GetRow(blockedRow))
            blockedIDs.insert(blockedRow.GetUInt(0));

        if (!blockedIDs.empty()) {
            std::vector<int32> filtered;
            for (int32 id : characters) {
                if (blockedIDs.find((uint32)id) == blockedIDs.end())
                    filtered.push_back(id);
            }
            characters.swap(filtered);
        }
    }

    // Check for optional roleMask parameter (used for corp mail role groups)
    uint32 roleMask = 0;
    if (call.byname.find("roleMask") != call.byname.end())
        roleMask = PyRep::IntegerValueU32(call.byname.find("roleMask")->second);

    int32 mailID = m_db.SendMail(
        sender, characters,
        listID.has_value() ? listID.value()->value() : -1,
        toCorpOrAllianceID.has_value() ? toCorpOrAllianceID.value()->value() : -1,
        title->content(), body->content(),
        isReplyTo->value(),
        isForwardedFrom->value(),
        roleMask
    );

    // push OnMailSent notification to online recipients
    if (mailID > 0) {
        for (int32 charID : characters) {
            Client* targetClient = sEntityList.FindClientByCharID(charID);
            if (targetClient != nullptr) {
                // Client's mailSvc.OnMailSent expects the 9-element payload used by
                // SelfEveMail (messageID, senderID, sentDate, senderID-str, toListID,
                // toCorpOrAllianceID, title, statusMask, extra{senderName}). A bare
                // 8-tuple without senderName is not parseable -> mail arrives in DB
                // but never shows in the recipient's inbox.
                PyTuple* payload = new PyTuple(9);
                payload->SetItem(0, new PyInt(mailID));
                payload->SetItem(1, new PyInt(sender));
                payload->SetItem(2, new PyLong(GetFileTimeNow()));
                payload->SetItem(3, new PyString(std::to_string(sender)));
                payload->SetItem(4, PyStatic.NewNone()); // toListID
                payload->SetItem(5, PyStatic.NewNone()); // toCorpOrAllianceID
                payload->SetItem(6, new PyString(title->content())); // title
                payload->SetItem(7, new PyInt(0));       // statusMask
                PyDict* extra = new PyDict();
                extra->SetItemString("senderName", new PyString(call.client->GetName()));
                payload->SetItem(8, extra);
                targetClient->SendNotification("OnMailSent", "clientID", payload, false);
            }
        }
    }

    return new PyInt(mailID);
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyString* body, PyBool* isReplyTo, PyBool* isForwardedFrom)
{
    // Client may send body as PyString* instead of PyWString*.
    // Convert and delegate to the PyWString* overload.
    PyWString* bodyW = new PyWString(body->content());
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, bodyW, isReplyTo, isForwardedFrom);
    PyDecRef(bodyW);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyWString* body, PyInt* isReplyTo, PyInt* isForwardedFrom)
{
    // Client sends isReplyTo/isForwardedFrom as PyInt* (0/1) instead of PyBool*.
    PyBool* isReplyB = new PyBool(isReplyTo->value() != 0);
    PyBool* isFwdB   = new PyBool(isForwardedFrom->value() != 0);
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, body, isReplyB, isFwdB);
    PyDecRef(isReplyB);
    PyDecRef(isFwdB);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyString* body, PyInt* isReplyTo, PyInt* isForwardedFrom)
{
    // Client sends isReplyTo/isForwardedFrom as PyInt* (0/1) instead of PyBool*.
    PyBool* isReplyB = new PyBool(isReplyTo->value() != 0);
    PyBool* isFwdB   = new PyBool(isForwardedFrom->value() != 0);
    PyWString* bodyW = new PyWString(body->content());
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, bodyW, isReplyB, isFwdB);
    PyDecRef(bodyW);
    PyDecRef(isReplyB);
    PyDecRef(isFwdB);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyWString* body, PyInt* isReplyTo, PyBool* isForwardedFrom)
{
    PyBool* isReplyB = new PyBool(isReplyTo->value() != 0);
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, body, isReplyB, isForwardedFrom);
    PyDecRef(isReplyB);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyString* body, PyInt* isReplyTo, PyBool* isForwardedFrom)
{
    PyBool* isReplyB = new PyBool(isReplyTo->value() != 0);
    PyWString* bodyW = new PyWString(body->content());
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, bodyW, isReplyB, isForwardedFrom);
    PyDecRef(bodyW);
    PyDecRef(isReplyB);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyWString* body, PyBool* isReplyTo, PyInt* isForwardedFrom)
{
    PyBool* isFwdB = new PyBool(isForwardedFrom->value() != 0);
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, body, isReplyTo, isFwdB);
    PyDecRef(isFwdB);
    return result;
}

PyResult MailMgrService::SendMail(PyCallArgs &call, PyList* toCharacterIDs, std::optional<PyInt*> listID, std::optional<PyInt*> toCorpOrAllianceID, PyWString* title, PyString* body, PyBool* isReplyTo, PyInt* isForwardedFrom)
{
    PyBool* isFwdB = new PyBool(isForwardedFrom->value() != 0);
    PyWString* bodyW = new PyWString(body->content());
    PyResult result = SendMail(call, toCharacterIDs, listID, toCorpOrAllianceID, title, bodyW, isReplyTo, isFwdB);
    PyDecRef(bodyW);
    PyDecRef(isFwdB);
    return result;
}

PyResult MailMgrService::PrimeOwners(PyCallArgs &call, PyList* ownerIDs)
{
    std::vector<int32> owners;

    PyList::const_iterator list_2_cur = ownerIDs->begin();
    for (size_t list_2_index(0); list_2_cur != ownerIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_SendMail failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        owners.push_back(t->value());
    }
    return ServiceDB::PrimeOwners(owners);
}

PyResult MailMgrService::SyncMail(PyCallArgs &call, std::optional<PyInt*> first, std::optional<PyInt*> second)
{
    int lastKnownId = 0;

    if (second.has_value())
    {
        // second = lowest cached messageID — only return mails newer than this
        lastKnownId = second.value()->value();
    }

    PyDict* dummy = new PyDict;
    dummy->SetItemString("oldMail", PyStatic.NewNone());
    dummy->SetItemString("newMail", m_db.GetNewMail(call.client->GetCharacterID(), lastKnownId));
    dummy->SetItemString("mailStatus", m_db.GetMailStatus(call.client->GetCharacterID()));
    return new PyObject("util.KeyVal", dummy);
}

PyResult MailMgrService::AssignLabels(PyCallArgs &call, PyList* messageIDs, PyInt* labelID)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.ApplyLabels(messageIds, labelID->value());

    return nullptr;
}

PyResult MailMgrService::CreateLabel(PyCallArgs &call, PyWString* name, std::optional<PyInt*> color)
{
    uint32 ret;
    if (m_db.CreateLabel(call.client->GetCharacterID(), name->content(), color.has_value() ? color.value()->value() : -1, ret))
        return new PyInt(ret);
    return nullptr;
}

PyResult MailMgrService::DeleteLabel(PyCallArgs &call, PyInt* labelID)
{
    m_db.DeleteLabel(call.client->GetCharacterID(), labelID->value());

    return nullptr;
}

PyResult MailMgrService::DeleteMail(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    for (int i = 0; i < messageIds.size(); i++) {
        int32 messageID = messageIds[i];
        m_db.DeleteMail(messageID);
    }

    return nullptr;
}

PyResult MailMgrService::EditLabel(PyCallArgs &call, PyInt* labelID, PyWString* name, std::optional<PyInt*> color)
{
    m_db.EditLabel(call.client->GetCharacterID(), labelID->value(), name->content(), color.has_value() ? color.value()->value() : -1);
    return nullptr;
}

PyResult MailMgrService::EmptyTrash(PyCallArgs &call)
{
    // @TODO: TEST
    m_db.EmptyTrash(call.client->GetCharacterID());
    return nullptr;
}

PyResult MailMgrService::GetBody(PyCallArgs &call, PyInt* messageID, PyBool* isUnread)
{
    if (!isUnread->value()) {
        m_db.SetMailUnread(messageID->value());
    } else {
        m_db.SetMailRead(messageID->value());
    }

    return m_db.GetMailBody(messageID->value());
}

PyResult MailMgrService::GetLabels(PyCallArgs &call)
{
    return m_db.GetLabels(call.client->GetCharacterID());
}

PyResult MailMgrService::GetMailHeaders(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> ids;
    PyList::const_iterator cur = messageIDs->begin();
    for (; cur != messageIDs->end(); ++cur) {
        if ((*cur)->IsInt())
            ids.push_back((*cur)->AsInt()->value());
    }
    if (ids.empty())
        return nullptr;
    return m_db.GetMailHeaders(call.client->GetCharacterID(), ids);
}

PyResult MailMgrService::MarkAllAsRead(PyCallArgs &call)
{
    m_db.MarkAllAsRead(call.client->GetCharacterID());

    return nullptr;
}

PyResult MailMgrService::MarkAllAsUnread(PyCallArgs &call)
{
    m_db.MarkAllAsUnread(call.client->GetCharacterID());
    return nullptr;
}

PyResult MailMgrService::MarkAsRead(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.SetMailsRead(messageIds);

    return nullptr;
}

PyResult MailMgrService::MarkAsReadByLabel(PyCallArgs &call, PyInt* labelID)
{
    m_db.MarkAllAsReadByLabel(call.client->GetCharacterID(), labelID->value());

    return nullptr;
}

PyResult MailMgrService::MarkAsReadByList(PyCallArgs &call, PyInt* listID)
{
    m_db.MarkAllAsReadByList(call.client->GetCharacterID(), listID->value());
    return nullptr;
}

PyResult MailMgrService::MarkAsUnread(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.SetMailsUnread(messageIds);

    return nullptr;
}

PyResult MailMgrService::MarkAsUnreadByLabel(PyCallArgs &call, PyInt* labelID)
{
    m_db.MarkAllAsUnreadByLabel(call.client->GetCharacterID(), labelID->value());

    return nullptr;
}

PyResult MailMgrService::MarkAsUnreadByList(PyCallArgs &call, PyInt* listID)
{
    m_db.MarkAllAsUnreadByList(call.client->GetCharacterID(), listID->value());
    return nullptr;
}

PyResult MailMgrService::MoveAllFromTrash(PyCallArgs &call)
{
    m_db.MoveAllFromTrash(call.client->GetCharacterID());
    return nullptr;
}

PyResult MailMgrService::MoveAllToTrash(PyCallArgs &call)
{
    m_db.MoveAllToTrash(call.client->GetCharacterID());
    return nullptr;
}

PyResult MailMgrService::MoveFromTrash(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.RemoveStatusMasks(messageIds, mailStatusMaskTrashed);

    return nullptr;
}

PyResult MailMgrService::MoveToTrash(PyCallArgs &call, PyList* messageIDs)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.ApplyStatusMasks(messageIds, mailStatusMaskTrashed);

    return nullptr;
}

PyResult MailMgrService::MoveToTrashByLabel(PyCallArgs &call, PyInt* labelID)
{
    m_db.MoveToTrashByLabel(call.client->GetCharacterID(), labelID->value());
    return nullptr;
}

PyResult MailMgrService::MoveToTrashByList(PyCallArgs &call, PyInt* listID)
{
    m_db.MoveToTrashByList(call.client->GetCharacterID(), listID->value());
    return nullptr;
}

PyResult MailMgrService::RemoveLabels(PyCallArgs &call, PyList* messageIDs, PyInt* labelID)
{
    std::vector<int32> messageIds;

    PyList::const_iterator list_2_cur = messageIDs->begin();
    for (size_t list_2_index(0); list_2_cur != messageIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_AssignLabels failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        messageIds.push_back(t->value());
    }

    m_db.RemoveLabels(messageIds, labelID->value());

    return nullptr;
}
