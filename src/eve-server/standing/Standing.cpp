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
    Updates:        Allan
*/

#include "standing/Standing.h"
#include "EntityList.h"
#include "system/CrimeWatch.h"

/*  re-write of standing system  -allan 10Apr15
 * see notes in StandingDB.cpp
 */

/*
 * STANDING__ERROR
 * STANDING__WARNING
 * STANDING__MESSAGE
 * STANDING__DEBUG
 * STANDING__INFO
 * STANDING__TRACE
 * STANDING__DUMP
 * STANDING__RSPDUMP
 */

Standing::Standing() :
    Service("standing2")
{
    this->Add("GetCharStandings", &Standing::GetCharStandings);
    this->Add("GetCorpStandings", &Standing::GetCorpStandings);
    this->Add("GetNPCNPCStandings", &Standing::GetNPCNPCStandings);
    this->Add("GetSecurityRating", &Standing::GetSecurityRating);
    this->Add("GetMyKillRights", &Standing::GetMyKillRights);
    this->Add("ActivateKillRight", &Standing::ActivateKillRight);
    this->Add("UpdateKillRight", &Standing::UpdateKillRight);
    this->Add("DeleteKillRight", &Standing::DeleteKillRight);
    this->Add("GetStandingTransactions", &Standing::GetStandingTransactions);
    this->Add("GetStandingCompositions", &Standing::GetStandingCompositions);
}

PyResult Standing::GetCharStandings(PyCallArgs &call) {
    return m_db.GetCharStandings(call.client);
}

PyResult Standing::GetCorpStandings(PyCallArgs &call) {
    return m_db.GetCorpStandings(call.client);
}

PyResult Standing::GetNPCNPCStandings(PyCallArgs &call) {
    return sStandingMgr.GetFactionStandings();
}

/** @todo  need to add a standing from corpCONCORD to any/all charID, corpID, allyID  for security rating (as seen in client code) */

PyResult Standing::GetSecurityRating(PyCallArgs &call, PyInt* ownerID) {
    CharacterRef cRef = sItemFactory.GetCharacterRef(ownerID->value());
    if  (cRef.get() == nullptr) {
        _log(STANDING__WARNING, "Character %u not found.", ownerID->value());
        return nullptr;
    }

    return new PyFloat( cRef->GetSecurityRating() );
}

PyResult Standing::GetMyKillRights(PyCallArgs &call) {
    _log(STANDING__MESSAGE,  "Standing::Handle_GetMyKillRights()");
    PyRep* result = m_kdb.GetKillRights(call.client->GetCharacterID(), call.client->GetCharacterID());
    if (result == nullptr) {
        PyTuple* empty = new PyTuple(2);
        empty->SetItem(0, new PyDict());
        empty->SetItem(1, new PyDict());
        return empty;
    }
    return result;
}

PyResult Standing::ActivateKillRight(PyCallArgs &call, PyInt* rightID) {
    // get targetID from DB before activating
    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT targetID, ownerID, price FROM chrKillRights WHERE rightID = %u AND used = 0", rightID->value());
    DBResultRow row;
    if (!res.GetRow(row)) {
        call.client->SendErrorMsg("Kill Right not found or already used.");
        return new PyBool(false);
    }
    uint32 targetID = row.GetInt(0);
    uint32 ownerID = row.GetInt(1);
    int64 price = row.GetInt64(2);

    if (!m_kdb.ActivateKillRight(rightID->value(), call.client->GetCharacterID())) {
        call.client->SendErrorMsg("Failed to activate Kill Right.");
        return new PyBool(false);
    }

    // pay the owner (buyer has already paid per EVE mechanic)
    if (price > 0) {
        DBerror err;
        sDatabase.RunQuery(err,
            " UPDATE chrCharacters SET balance = balance - %" PRIi64 " WHERE characterID = %u",
            price, call.client->GetCharacterID());
        sDatabase.RunQuery(err,
            " UPDATE chrCharacters SET balance = balance + %" PRIi64 " WHERE characterID = %u",
            price, ownerID);
    }

    // set Limited Engagement on target
    Client* targetClient = sEntityList.FindClientByCharID(targetID);
    if (targetClient != nullptr && targetClient->GetCrimeWatch() != nullptr)
        targetClient->GetCrimeWatch()->SetLimitedEngagement();

    call.client->SendNotifyMsg("Kill Right activated. %s is now a legal target.", targetClient ? targetClient->GetName() : "Target");
    return new PyBool(true);
}

PyResult Standing::UpdateKillRight(PyCallArgs &call, PyInt* rightID, PyLong* price, std::optional<PyInt*> accessMask) {
    uint8 mask = accessMask.has_value() ? accessMask.value()->value() : 0;
    if (m_kdb.UpdateKillRight(rightID->value(), price->value(), mask)) {
        call.client->SendNotifyMsg("Kill Right updated.");
        return new PyBool(true);
    }
    return new PyBool(false);
}

PyResult Standing::DeleteKillRight(PyCallArgs &call, PyInt* rightID) {
    if (m_kdb.DeleteKillRight(rightID->value())) {
        call.client->SendNotifyMsg("Kill Right deleted.");
        return new PyBool(true);
    }
    return new PyBool(false);
}

PyResult Standing::GetStandingTransactions(PyCallArgs &call, PyInt* fromID, PyInt* toID, PyInt* direction, std::optional<PyInt*> eventID, std::optional<PyInt*> eventType, std::optional<PyLong*> eventDateTime) {
    // data = sm.RemoteSvc('standing2').GetStandingTransactions(fromID, toID, direction, eventID, eventType, eventDateTime)
    _log(STANDING__MESSAGE,  "Standing::Handle_GetStandingTransactions()");
    call.Dump(STANDING__DUMP);

    return m_db.GetStandingTransactions(fromID->value(), toID->value());
}

PyResult Standing::GetStandingCompositions(PyCallArgs &call, PyInt* fromID, PyInt* toID) {
/**  no clue what this is yet
                self.sr.data = sm.RemoteSvc('standing2').GetStandingCompositions(fromID, toID)
            if self.sr.data:
                prior = 0.0
                for each in self.sr.data:
                    if each.ownerID == fromID:
                        prior = each.standing
                        */
    _log(STANDING__MESSAGE,  "Standing::Handle_GetStandingCompositions()");
    call.Dump(STANDING__DUMP);

    return m_db.GetStandingCompositions(fromID->value(), toID->value());
}
