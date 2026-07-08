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
    Rewrite:    Allan
*/

#include "eve-server.h"


#include "StaticDataMgr.h"
#include "EVE_Mail.h"
#include "cache/ObjCacheService.h"
#include "account/AccountDB.h"
#include "account/AccountService.h"
#include "corporation/BillMgr.h"

BillMgr::BillMgr() :
    Service("billMgr")
{
    this->Add("CharPayBill", &BillMgr::CharPayBill);
    this->Add("CharGetBills", &BillMgr::CharGetBills);
    this->Add("CharGetBillsReceivable", &BillMgr::CharGetBillsReceivable);
    this->Add("GetBillTypes", &BillMgr::GetBillTypes);
    this->Add("PayCorporationBill", &BillMgr::PayCorporationBill);
    this->Add("GetCorporationBills", &BillMgr::GetCorporationBills);
    this->Add("GetCorporationBillsReceivable", &BillMgr::GetCorporationBillsReceivable);
    this->Add("GetAutomaticPaySettings", &BillMgr::GetAutomaticPaySettings);
    this->Add("SendAutomaticPaySettings", &BillMgr::SendAutomaticPaySettings);
}

PyResult BillMgr::GetBillTypes(PyCallArgs& call) {
    return sDataMgr.GetBillTypes();
}

PyResult BillMgr::GetCorporationBills(PyCallArgs &call) {
    return m_db.GetCorporationBills(call.client->GetCorporationID(), true);
}

PyResult BillMgr::GetCorporationBillsReceivable(PyCallArgs &call) {
    return m_db.GetCorporationBills(call.client->GetCorporationID(), false);
}


PyResult BillMgr::CharPayBill(PyCallArgs &call, PyInt* billID) {
    _log(CORP__CALL, "BillMgr::Handle_CharPayBill() size=%lli", call.tuple->size());
    uint32 charID = call.client->GetCharacterID();
    uint32 bill = billID->value();

    DBQueryResult res;
    DBResultRow row;
    if (!sDatabase.RunQuery(res,
        "SELECT debtorID, creditorID, billTypeID, amount FROM billsPayable WHERE billID = %u AND paid = 0", bill))
    {
        call.client->SendErrorMsg("Bill not found or already paid.");
        return PyStatic.NewNone();
    }
    if (!res.GetRow(row) or row.GetUInt(0) != charID) {
        call.client->SendErrorMsg("Bill not found.");
        return PyStatic.NewNone();
    }
    uint32 creditorID = row.GetUInt(1);
    uint32 billType = row.GetUInt(2);
    double amount = row.GetDouble(3);

    if (call.client->GetBalance(Account::CreditType::ISK) < amount) {
        call.client->SendErrorMsg("Insufficient funds to pay this bill.");
        return PyStatic.NewNone();
    }

    // Transfer funds and create journal entry
    AccountService::TransferFunds(charID, creditorID, amount,
        "Bill payment", billType, bill, Account::KeyType::Cash, Account::KeyType::Cash, call.client);

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE billsPayable SET paid = 1 WHERE billID = %u", bill);

    _log(CORP__MESSAGE, "BillMgr::CharPayBill() - char %u paid bill %u for %.2f ISK", charID, bill, amount);

    // create notification for creditor
    PyDict* data = new PyDict();
        data->SetItemString("billID", new PyInt(bill));
        data->SetItemString("debtorID", new PyInt(charID));
        data->SetItemString("amount", new PyFloat(amount));
    sEntityList.CreateNotification(creditorID, Notify::Types::BillPaidChar, charID, data);

    return PyStatic.NewNone();
}

PyResult BillMgr::CharGetBills(PyCallArgs &call) {
    _log(CORP__CALL, "BillMgr::Handle_CharGetBills() size=%lli", call.tuple->size());
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT billID, billTypeID, debtorID, creditorID, amount, dueDateTime, interest, "
        "externalID, paid FROM billsPayable WHERE debtorID = %u", call.client->GetCharacterID()))
    {
        codelog(DATABASE__ERROR, "Error in CharGetBills query: %s", res.error.c_str());
        return new PyList();
    }
    return DBResultToRowset(res);
}

PyResult BillMgr::CharGetBillsReceivable(PyCallArgs &call) {
    _log(CORP__CALL, "BillMgr::Handle_CharGetBillsReceivable() size=%lli", call.tuple->size());
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT billID, billTypeID, debtorID, creditorID, amount, dueDateTime, interest, "
        "externalID, paid FROM billsReceivable WHERE creditorID = %u", call.client->GetCharacterID()))
    {
        codelog(DATABASE__ERROR, "Error in CharGetBillsReceivable query: %s", res.error.c_str());
        return new PyList();
    }
    return DBResultToRowset(res);
}

PyResult BillMgr::PayCorporationBill(PyCallArgs &call, PyInt* billID) {
    _log(CORP__CALL, "BillMgr::Handle_PayCorporationBill() size=%lli", call.tuple->size());

    uint32 corpID = call.client->GetCorporationID();
    uint32 bill = billID->value();

    uint32 accountKey = call.client->GetCorpAccountKey();
    if (call.byname.find("fromAccountKey") != call.byname.end())
        accountKey = PyRep::IntegerValueU32(call.byname.find("fromAccountKey")->second);

    DBQueryResult res;
    DBResultRow row;
    if (!sDatabase.RunQuery(res,
        "SELECT debtorID, creditorID, billTypeID, amount FROM billsPayable WHERE billID = %u AND paid = 0", bill))
    {
        call.client->SendErrorMsg("Bill not found or already paid.");
        return PyStatic.NewNone();
    }
    if (!res.GetRow(row) or row.GetUInt(0) != corpID) {
        call.client->SendErrorMsg("Bill not found.");
        return PyStatic.NewNone();
    }
    uint32 creditorID = row.GetUInt(1);
    uint32 billType = row.GetUInt(2);
    double amount = row.GetDouble(3);

    // Check corp wallet balance
    double balance = AccountDB::GetCorpBalance(corpID, accountKey);
    if (balance < amount) {
        call.client->SendErrorMsg("Insufficient corporation funds to pay this bill.");
        return PyStatic.NewNone();
    }

    // Transfer from corp wallet to creditor
    AccountService::TransferFunds(corpID, creditorID, amount,
        "Corporation bill payment", billType, bill, accountKey, Account::KeyType::Cash, call.client);

    DBerror err;
    sDatabase.RunQuery(err, "UPDATE billsPayable SET paid = 1 WHERE billID = %u", bill);

    _log(CORP__MESSAGE, "BillMgr::PayCorporationBill() - corp %u paid bill %u for %.2f ISK from account %u", corpID, bill, amount, accountKey);
    return PyStatic.NewNone();
}


PyResult BillMgr::SendAutomaticPaySettings(PyCallArgs &call, PyDict* automaticPaymentSettings) {
    _log(CORP__CALL, "BillMgr::Handle_SendAutomaticPaySettings() size=%lli", call.tuple->size());

    uint32 corpID = call.client->GetCorporationID();
    // Format: { corpID: { billTypeID: bool, ..., 'divisionID': int } }
    // We extract the inner dict for our corpID
    PyDict* settings = nullptr;
    PyDict::const_iterator itr = automaticPaymentSettings->begin();
    if (itr != automaticPaymentSettings->end()) {
        PyRep* val = itr->second;
        if (val->IsDict())
            settings = val->AsDict();
    }
    if (settings == nullptr)
        return PyStatic.NewNone();

    bool market = false, rental = false, broker = false, war = false, alliance = false, sov = false;
    PyDict::const_iterator sitr, send = settings->end();
    for (sitr = settings->begin(); sitr != send; ++sitr) {
        if (!sitr->first->IsInt() or !sitr->second->IsBool())
            continue;
        uint32 type = sitr->first->AsInt()->value();
        bool val = sitr->second->AsBool()->value();
        switch (type) {
            case Corp::BillType::MarketFine:              market = val; break;
            case Corp::BillType::RentalBill:              rental = val; break;
            case Corp::BillType::BrokerBill:              broker = val; break;
            case Corp::BillType::WarBill:                  war = val; break;
            case Corp::BillType::AllianceMaintainanceBill: alliance = val; break;
            case Corp::BillType::SovereigntyMarker:        sov = val; break;
        }
    }

    DBerror err;
    sDatabase.RunQuery(err,
        "REPLACE INTO crpAutoPay (corporationID, market, rental, broker, war, alliance, sov) "
        "VALUES (%u, %u, %u, %u, %u, %u, %u)",
        corpID, market, rental, broker, war, alliance, sov);

    _log(CORP__MESSAGE, "BillMgr::SendAutomaticPaySettings() - corp %u saved", corpID);
    return PyStatic.NewNone();
}

PyResult BillMgr::GetAutomaticPaySettings(PyCallArgs &call) {
    //    ambSettings = sm.RemoteSvc('billMgr').GetAutomaticPaySettings()
    // returns t/f for bill types
    DBQueryResult res;
    m_db.GetAutoPay(call.client->GetCorporationID(), res);

    DBResultRow row;
    PyDict* sets = new PyDict();
    if (res.GetRow(row)) {
        sets->SetItem(new PyInt(Corp::BillType::MarketFine), new PyBool(row.GetBool(0)));
        sets->SetItem(new PyInt(Corp::BillType::RentalBill), new PyBool(row.GetBool(1)));
        sets->SetItem(new PyInt(Corp::BillType::BrokerBill), new PyBool(row.GetBool(2)));
        sets->SetItem(new PyInt(Corp::BillType::WarBill), new PyBool(row.GetBool(3)));
        if (call.client->GetAllianceID())
            sets->SetItem(new PyInt(Corp::BillType::AllianceMaintainanceBill), new PyBool(row.GetBool(4)));
        sets->SetItem(new PyInt(Corp::BillType::SovereigntyMarker), new PyBool(row.GetBool(5)));
    }

    PyDict* dict = new PyDict();
        dict->SetItem(new PyInt(call.client->GetCorporationID()), sets);

    if (is_log_enabled(CORP__RSP_DUMP))
        dict->Dump(CORP__RSP_DUMP, "");

    return dict;
}


/*
 *        [PyString "OnNotificationReceived"]
 *        [PyList 0 items]
 *        [PyString "clientID"]
 *    [PyInt 5654387]
 *    [PyTuple 1 items]
 *      [PyTuple 2 items]
 *        [PyInt 0]
 *        [PySubStream 168 bytes]
 *          [PyTuple 2 items]
 *            [PyInt 0]
 *            [PyTuple 2 items]
 *              [PyInt 1]
 *              [PyTuple 5 items]
 *                [PyInt 342402174]
 *                [PyInt 10]            << Notify::Types::CorpAllBill
 *                [PyInt 1000167]
 *                [PyIntegerVar 129492968400000000]
 *                [PyDict 8 kvp]
 *                  [PyString "debtorID"]
 *                  [PyInt 98038978]
 *                  [PyString "creditorID"]
 *                  [PyInt 1000167]
 *                  [PyString "billTypeID"]
 *                  [PyInt 2]
 *                  [PyString "amount"]
 *                  [PyInt 981907]
 *                  [PyString "externalID2"]
 *                  [PyInt 60014683]
 *                  [PyString "externalID"]
 *                  [PyInt 27]
 *                  [PyString "currentDate"]
 *                  [PyIntegerVar 129492968683459696]
 *                  [PyString "dueDate"]
 *                  [PyIntegerVar 129518888683422295]
 *    [PyDict 1 kvp]
 *      [PyString "sn"]
 *      [PyIntegerVar 4]
 */

/*
// OnBillReceived, an essentially empty tuple, just to tell the client that there is something,
// maybe for blinking purpose?
OnBillReceived N_obr;   // this is in Wallet.xmlp
PyTuple * res5 = N_obr.Encode();
//call.client->SendNotification("OnBillReceived", "*corpid&corprole", &res5, false);
// Why do we create a bill, when the office is already paid? Maybe that's why it's empty...


// OnMessage notification, the LSC packet NotifyOnMessage can be used, along with the StoreNewEVEMail
// Who to send notification? corpRoleJuniorAccountant and equiv? atm it's enough to send it to the renter
// TODO: get the correct evemail content from somewhere
// TODO: send it to every corp member who's affected by it. corpRoleAccountant, corpRoleJuniorAccountant or equiv
m_manager->lsc_service->SendMail(
    m_db.GetStationCorporationCEO(oInfo.stationID),
                                 call.client->GetCharacterID(),
                                 "Bill issued",
                                 "Bill issued for renting an office");

*/