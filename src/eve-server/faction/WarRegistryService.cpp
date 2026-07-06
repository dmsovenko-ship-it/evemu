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
    Updates:    Allan
*/

#include "eve-server.h"

#include "account/AccountDB.h"
#include "EVE_Corp.h"
#include "faction/WarRegistryService.h"
#include "services/ServiceManager.h"

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

WarRegistryService::WarRegistryService(EVEServiceManager& mgr) :
    BindableService("warRegistry", mgr)
{
    // Ensure warRegistry table exists
    DBerror err;
    sDatabase.RunQuery(err,
        "CREATE TABLE IF NOT EXISTS warRegistry ("
        "  warID int(10) NOT NULL AUTO_INCREMENT,"
        "  declaredByID int(10) NOT NULL DEFAULT '0',"
        "  againstID int(10) NOT NULL DEFAULT '0',"
        "  timeDeclared bigint(20) NOT NULL DEFAULT '0',"
        "  timeFinished bigint(20) NOT NULL DEFAULT '0',"
        "  retracted bigint(20) NOT NULL DEFAULT '0',"
        "  retractedBy int(10) NOT NULL DEFAULT '0',"
        "  billID int(10) NOT NULL DEFAULT '0',"
        "  mutual tinyint(1) NOT NULL DEFAULT '0',"
        "  PRIMARY KEY (warID)"
        ") ENGINE=MyISAM");
}

BoundDispatcher* WarRegistryService::BindObject(Client* client, PyRep* bindParameters) {
    Call_TwoIntegerArgs args;

    if (args.Decode(bindParameters->Clone()) == false) {
        codelog(SERVICE__ERROR, "%s: Failed to decode bind args.", GetName().c_str());
        return nullptr;
    }

    uint32 corporationID = args.arg1;
    auto it = this->m_instances.find (corporationID);

    if (it != this->m_instances.end ())
        return it->second;

    WarRegistryBound* bound = new WarRegistryBound(args.arg1, this->GetServiceManager (), *this);

    this->m_instances.insert_or_assign (corporationID, bound);

    return bound;
}

void WarRegistryService::BoundReleased (WarRegistryBound* bound) {
    auto it = this->m_instances.find (bound->GetCorporationID());

    if (it == this->m_instances.end ())
        return;

    this->m_instances.erase (it);
}

WarRegistryBound::WarRegistryBound(uint32 corporationID, EVEServiceManager& mgr, WarRegistryService& parent) :
    EVEBoundObject(mgr, parent),
    mCorporationID(corporationID)
{
    this->Add("GetWars", &WarRegistryBound::GetWars);
    this->Add("RetractWar", &WarRegistryBound::RetractWar);
    this->Add("DeclareWarAgainst", &WarRegistryBound::DeclareWarAgainst);
    this->Add("ChangeMutualWarFlag", &WarRegistryBound::ChangeMutualWarFlag);
    this->Add("GetCostOfWarAgainst", &WarRegistryBound::GetCostOfWarAgainst);
}

PyResult WarRegistryBound::GetWars(PyCallArgs& args, PyInt* ownerID, std::optional<PyInt*> forceRefresh) {
    _log(FACWAR__CALL, "WarRegistryBound::Handle_GetWars() size=%lli", args.tuple->size());

    uint32 id = ownerID->value();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT warID, declaredByID, againstID, timeDeclared, timeFinished, "
        "retracted, retractedBy, billID, mutual "
        "FROM warRegistry WHERE declaredByID = %u OR againstID = %u", id, id))
    {
        return new PyList();
    }

    return DBResultToIndexRowset(res, "warID");
}

PyResult WarRegistryBound::RetractWar(PyCallArgs& args, PyInt* againstID) {
    _log(FACWAR__CALL, "WarRegistryBound::Handle_RetractWar() size=%lli", args.tuple->size());

    uint32 attackerID = args.client->GetCorporationID();
    uint32 targetID = againstID->value();

    // Find active war where we are the attacker
    DBQueryResult res;
    DBResultRow row;
    if (!sDatabase.RunQuery(res,
        "SELECT warID FROM warRegistry "
        "WHERE declaredByID = %u AND againstID = %u AND retracted = 0", attackerID, targetID))
    {
        args.client->SendErrorMsg("Active war not found.");
        return PyStatic.NewNone();
    }
    if (!res.GetRow(row)) {
        args.client->SendErrorMsg("Active war not found.");
        return PyStatic.NewNone();
    }
    uint32 warID = row.GetUInt(0);
    EndWar(warID, attackerID);

    _log(FACWAR__MESSAGE, "WarRegistryBound::RetractWar() - corp %u retracted war %u against %u", attackerID, warID, targetID);
    return PyStatic.NewNone();
}

PyResult WarRegistryBound::DeclareWarAgainst(PyCallArgs& args, PyInt* againstID) {
    _log(FACWAR__CALL, "WarRegistryBound::Handle_DeclareWarAgainst() size=%lli", args.tuple->size());

    uint32 attackerID = args.client->GetCorporationID();
    uint32 targetID = againstID->value();

    // Check if already at war
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT warID FROM warRegistry WHERE ((declaredByID = %u AND againstID = %u) "
        "OR (declaredByID = %u AND againstID = %u)) AND retracted = 0",
        attackerID, targetID, targetID, attackerID))
    {
        DBResultRow row;
        if (res.GetRow(row)) {
            args.client->SendErrorMsg("Already at war with this target.");
            return PyStatic.NewNone();
        }
    }

    // Get war cost and check balance
    double warCost = 50000000.0; // 50M ISK default (half of typical EVE cost for dev/testing)

    if (args.client->GetBalance(Account::CreditType::ISK) < warCost) {
        args.client->SendErrorMsg("Insufficient funds to declare war.");
        return PyStatic.NewNone();
    }

    // Deduct ISK (goes to NPC Concord)
    args.client->AddBalance(-warCost);
    // Create journal entry for war fee
    AccountDB::AddJournalEntry(args.client->GetCharacterID(),
        Journal::EntryType::WarFee,
        args.client->GetCharacterID(), 1, // 1 = EVE System
        Account::CreditType::ISK, Account::KeyType::Cash,
        -warCost, args.client->GetBalance(Account::CreditType::ISK),
        "War declaration fee", 0);

    // Create war record
    uint32 warID = CreateWarRecord(attackerID, targetID);

    // Create a bill for the war (weekly maintenance)
    uint32 billID = 0;
    double billAmount = warCost * 0.25; // 25% of declaration cost per week
    double dueTime = (double)(GetFileTimeNow() + 7LL * 24LL * 60LL * 60LL * 10000000LL); // 1 week in Win32 FILETIME
    DBerror dberr;
    sDatabase.RunQueryLID(dberr, billID,
        "INSERT INTO billsPayable (billTypeID, debtorID, creditorID, amount, dueDateTime, interest, externalID, externalID2, paid) "
        "VALUES (%u, %u, 1, %.2f, %.0f, 0, %u, 0, 0)",
        Corp::BillType::WarBill, attackerID, billAmount, dueTime, warID);

    // Update war record with billID
    sDatabase.RunQuery(dberr,
        "UPDATE warRegistry SET billID = %u WHERE warID = %u", billID, warID);

    _log(FACWAR__MESSAGE, "WarRegistryBound::DeclareWarAgainst() - corp %u declared war %u on %u, cost %.2f", attackerID, warID, targetID, warCost);
    return PyStatic.NewNone();
}

PyResult WarRegistryBound::ChangeMutualWarFlag(PyCallArgs& args, PyInt* warID, PyBool* mutual) {
    _log(FACWAR__CALL, "WarRegistryBound::Handle_ChangeMutualWarFlag() size=%lli", args.tuple->size());

    uint32 id = warID->value();
    bool val = mutual->value();

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE warRegistry SET mutual = %u WHERE warID = %u", val, id);

    _log(FACWAR__MESSAGE, "WarRegistryBound::ChangeMutualWarFlag() - war %u mutual=%u", id, val);
    return PyStatic.NewTrue();
}

PyResult WarRegistryBound::GetCostOfWarAgainst(PyCallArgs& args, PyInt* ownerID) {
    _log(FACWAR__CALL, "WarRegistryBound::Handle_GetCostOfWarAgainst() size=%lli", args.tuple->size());
    // Return fixed cost for now (50000000 ISK)
    return new PyFloat(50000000.0);
}

uint32 WarRegistryBound::CreateWarRecord(uint32 declaredByID, uint32 againstID) {
    DBerror err;
    uint32 warID = 0;
    double now = (double)GetFileTimeNow();
    sDatabase.RunQueryLID(err, warID,
        "INSERT INTO warRegistry (declaredByID, againstID, timeDeclared, mutual) "
        "VALUES (%u, %u, %.0f, 0)", declaredByID, againstID, now);

    _log(FACWAR__TRACE, "WarRegistryBound::CreateWarRecord() - created war %u", warID);
    return warID;
}

void WarRegistryBound::EndWar(uint32 warID, uint32 retractedBy) {
    double now = (double)GetFileTimeNow();
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE warRegistry SET retracted = %.0f, retractedBy = %u, timeFinished = %.0f "
        "WHERE warID = %u",
        now, retractedBy, now, warID);
}
