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
    Author:     Zhur, Allan
*/

#include "eve-server.h"

#include "EVEServerConfig.h"


#include "cache/ObjCacheService.h"
#include "corporation/CorpRegistryService.h"
#include "alliance/AllianceDB.h"
#include "corporation/CorpRegistryBound.h"

/*
 * CORP__ERROR
 * CORP__WARNING
 * CORP__INFO
 * CORP__MESSAGE
 * CORP__TRACE
 * CORP__CALL
 * CORP__CALL_DUMP
 * CORP__RSP_DUMP
 * CORP__DB_ERROR
 * CORP__DB_WARNING
 * CORP__DB_INFO
 * CORP__DB_MESSAGE
 */

CorpRegistryService::CorpRegistryService(EVEServiceManager& mgr) :
    BindableService("corpRegistry", mgr)
{
    /** @note: all of these are skeleton code only */
    this->Add("CreateAlliance", &CorpRegistryService::CreateAlliance);
    this->Add("GetRecentKillsAndLosses", &CorpRegistryService::GetRecentKillsAndLosses);
    this->Add("GetCorporateContacts", &CorpRegistryService::GetCorporateContacts);
    this->Add("AddCorporateContact", &CorpRegistryService::AddCorporateContact);
    this->Add("EditCorporateContact", &CorpRegistryService::EditCorporateContact);
    this->Add("RemoveCorporateContacts", &CorpRegistryService::RemoveCorporateContacts);
    this->Add("EditContactsRelationshipID", &CorpRegistryService::EditContactsRelationshipID);
    this->Add("GetLabels", &CorpRegistryService::GetLabels);
    this->Add("CreateLabel", &CorpRegistryService::CreateLabel);
    this->Add("DeleteLabel", &CorpRegistryService::DeleteLabel);
    this->Add("EditLabel", &CorpRegistryService::EditLabel);
    this->Add("AssignLabels", &CorpRegistryService::AssignLabels);
    this->Add("RemoveLabels", &CorpRegistryService::RemoveLabels);
    this->Add("ResignFromCEO", &CorpRegistryService::ResignFromCEO);
}

BoundDispatcher* CorpRegistryService::BindObject(Client* client, PyRep* bindParameters)
{
    if (!bindParameters->IsTuple()){
        sLog.Error( "CorpRegistryService::CreateBoundObject", "%s: bind_args is not tuple: '%s'. ", client->GetName(), bindParameters->TypeString() );
        client->SendErrorMsg("Could not bind object for Corp Registry.  Ref: ServerError 02808.");
        return nullptr;
    }

    uint32 corporationID = PyRep::IntegerValue(bindParameters->AsTuple()->GetItem(0));
    auto it = this->m_instances.find (corporationID);

    if (it != this->m_instances.end ())
        return it->second;

    CorpRegistryBound* bound = new CorpRegistryBound(this->GetServiceManager(), *this, m_db, corporationID);

    this->m_instances.insert_or_assign (corporationID, bound);

    return bound;
}

void CorpRegistryService::BoundReleased (CorpRegistryBound* bound) {
    auto it = this->m_instances.find (bound->GetCorporationID());

    if (it == this->m_instances.end ())
        return;

    this->m_instances.erase (it);
}

PyResult CorpRegistryService::GetCorporateContacts(PyCallArgs &call)
{
    return m_db.GetContacts(call.client->GetCorporationID());
}

/**     ***********************************************************************
 * @note   these below are partially coded
 */



/**     ***********************************************************************
 * @note   these do absolutely nothing at this time....
 */

PyResult CorpRegistryService::ResignFromCEO(PyCallArgs &call, PyInt* newCeoID) {
    //    self.GetCorpRegistry().ResignFromCEO(newCeoID)
    _log(CORP__CALL, "CorpRegistryService::Handle_ResignFromCEO()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}

PyResult CorpRegistryService::CreateAlliance(PyCallArgs &call, PyRep* allianceName, PyRep* shortName, PyRep* description, PyRep* url) {
    _log(CORP__CALL, "CorpRegistryService::Handle_CreateAlliance()");
    call.Dump(CORP__CALL_DUMP);

    Client* pClient = call.client;
    if (pClient == nullptr) return PyStatic.NewNone();

    // Basic validation
    std::string nameStr = allianceName->AsString()->content();
    std::string shortStr = shortName->AsString()->content();

    if (nameStr.empty() || shortStr.empty()) {
        pClient->SendNotifyMsg("Alliance name and short name are required.");
        return PyStatic.NewNone();
    }

    // Check corp CEO status and wallet
    uint32 corpID = pClient->GetCorporationID();
    uint32 charID = pClient->GetCharacterID();

    DBQueryResult ceoRes;
    if (sDatabase.RunQuery(ceoRes,
        "SELECT ceoID, corporationID FROM crpCorporations WHERE corporationID = %u AND ceoID = %u",
        corpID, charID))
    {
        if (ceoRes.GetRowCount() == 0) {
            pClient->SendNotifyMsg("Only the CEO can create an alliance.");
            return new PyInt(-1);
        }
    }

    // Check wallet balance (alliance creation costs 1M ISK in EVE: sovereignty update cost)
    if (!pClient->AddBalance(-1000000.0)) {
        pClient->SendNotifyMsg("Insufficient ISK. Alliance creation costs 1,000,000 ISK.");
        return new PyInt(-1);
    }

    AllianceDB a_db;
    uint32 allyID = 0;
    uint32 outCorpID = corpID;
    if (!a_db.CreateAlliance(nameStr, shortStr,
        description ? description->AsString()->content() : "",
        url ? url->AsString()->content() : "",
        pClient, allyID, outCorpID))
    {
        pClient->AddBalance(1000000.0);
        pClient->SendNotifyMsg("Failed to create alliance. Name may already be taken.");
        return new PyInt(-1);
    }

    pClient->SendNotifyMsg("Alliance %s created successfully.", nameStr.c_str());
    return new PyInt(allyID);
}

PyResult CorpRegistryService::GetRecentKillsAndLosses(PyCallArgs &call) {
    _log(CORP__CALL, "CorpRegistryService::Handle_GetRecentKillsAndLosses()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}


PyResult CorpRegistryService::AddCorporateContact(PyCallArgs &call, PyInt* contactID, PyInt* relationshipID) {
    _log(CORP__CALL, "CorpRegistryService::AddCorporateContact()");
    call.Dump(CORP__CALL_DUMP);

    if (!(call.client->GetCorpRole() & 0x401)) {
        call.client->SendErrorMsg("You need the Director or Diplomat role to manage corporation contacts.");
        return nullptr;
    }

    m_db.AddContact(call.client->GetCorporationID(), contactID->value(), relationshipID->value());
    return nullptr;
}

PyResult CorpRegistryService::EditCorporateContact(PyCallArgs &call, PyInt* contactID, PyInt* relationshipID) {
    _log(CORP__CALL, "CorpRegistryService::EditCorporateContact()");
    call.Dump(CORP__CALL_DUMP);

    if (!(call.client->GetCorpRole() & 0x401)) {
        call.client->SendErrorMsg("You need the Director or Diplomat role to manage corporation contacts.");
        return nullptr;
    }

    m_db.UpdateContact(relationshipID->value(), contactID->value(), call.client->GetCorporationID());
    return nullptr;
}

PyResult CorpRegistryService::RemoveCorporateContacts(PyCallArgs &call, PyList* contactIDs) {
    _log(CORP__CALL, "CorpRegistryService::RemoveCorporateContacts()");
    call.Dump(CORP__CALL_DUMP);

    if (!(call.client->GetCorpRole() & 0x401)) {
        call.client->SendErrorMsg("You need the Director or Diplomat role to manage corporation contacts.");
        return nullptr;
    }

    uint32 corpID = call.client->GetCorporationID();
    for (PyList::const_iterator itr = contactIDs->begin(); itr != contactIDs->end(); ++itr)
        m_db.RemoveContact(PyRep::IntegerValueU32(*itr), corpID);
    return nullptr;
}

PyResult CorpRegistryService::EditContactsRelationshipID(PyCallArgs &call, PyList* contactIDs, PyInt* relationshipID) {
    _log(CORP__CALL, "CorpRegistryService::EditContactsRelationshipID()");
    call.Dump(CORP__CALL_DUMP);

    if (!(call.client->GetCorpRole() & 0x401)) {
        call.client->SendErrorMsg("You need the Director or Diplomat role to manage corporation contacts.");
        return nullptr;
    }

    uint32 corpID = call.client->GetCorporationID();
    for (PyList::const_iterator itr = contactIDs->begin(); itr != contactIDs->end(); ++itr)
        m_db.UpdateContact(relationshipID->value(), PyRep::IntegerValueU32(*itr), corpID);
    return nullptr;
}

PyResult CorpRegistryService::GetLabels(PyCallArgs &call) {
    _log(CORP__CALL, "CorpRegistryService::Handle_GetLabels()");
    call.Dump(CORP__CALL_DUMP);

    return m_db.GetLabels(call.client->GetCorporationID());
}

PyResult CorpRegistryService::CreateLabel(PyCallArgs &call, PyWString* name, std::optional <PyInt*> color) {
 /*    def CreateLabel(self, name, color = 0):
  *        return self.GetCorpRegistry().CreateLabel(name, color)
  */
    _log(CORP__CALL, "CorpRegistryService::Handle_CreateLabel()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}

PyResult CorpRegistryService::DeleteLabel(PyCallArgs &call, PyInt* labelID) {
 /*    def DeleteLabel(self, labelID):
  *        self.GetCorpRegistry().DeleteLabel(labelID)
  */
    _log(CORP__CALL, "CorpRegistryService::Handle_DeleteLabel()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}

PyResult CorpRegistryService::EditLabel(PyCallArgs &call, PyInt* labelID, std::optional <PyWString*> name, std::optional <PyInt*> color) {
 /*    def EditLabel(self, labelID, name = None, color = None):
  *        self.GetCorpRegistry().EditLabel(labelID, name, color)
  */
    _log(CORP__CALL, "CorpRegistryService::Handle_EditLabel()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}

PyResult CorpRegistryService::AssignLabels(PyCallArgs &call, PyList* contactIDs, PyInt* labelMask) {
 /*    def AssignLabels(self, contactIDs, labelMask):
  *        self.GetCorpRegistry().AssignLabels(contactIDs, labelMask)
  */
    _log(CORP__CALL, "CorpRegistryService::Handle_AssignLabels()");
    call.Dump(CORP__CALL_DUMP);

    return nullptr;
}

PyResult CorpRegistryService::RemoveLabels(PyCallArgs &call, PyList* contactIDs, PyInt* labelMask) {
/*    def RemoveLabels(self, contactIDs, labelMask):
 *        self.GetCorpRegistry().RemoveLabels(contactIDs, labelMask)
 */
    _log(CORP__CALL, "CorpRegistryService::Handle_RemoveLabels()");
    call.Dump(CORP__CALL_DUMP);


    return nullptr;
}
