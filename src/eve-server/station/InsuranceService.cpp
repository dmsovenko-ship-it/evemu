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
    Author:     Zhur
    Rewrite:    Allan
*/

#include "eve-server.h"



#include "Client.h"
#include "account/AccountService.h"
#include "chat/LSCService.h"
#include "station/InsuranceService.h"
#include "system/SystemEntity.h"
#include "system/SystemManager.h"
#include "services/ServiceManager.h"

/* TODO:
* - set ship->basePrice to use history market value from marketProxy!
*/

InsuranceService::InsuranceService(EVEServiceManager& mgr) :
    BindableService("insuranceSvc", mgr)
{
    this->Add("GetContractForShip", &InsuranceService::GetContractForShip);
    this->Add("GetInsurancePrice", &InsuranceService::GetInsurancePrice);
	//SetSessionCheck?
}

BoundDispatcher* InsuranceService::BindObject(Client* client, PyRep* bindParameters) {
    return new InsuranceBound(this->GetServiceManager(), *this, &m_db );
}

void InsuranceService::BoundReleased (InsuranceBound* bound) {

}

PyResult InsuranceService::GetContractForShip(PyCallArgs& call, PyInt* shipID) {
    return m_db.GetInsuranceByShipID(shipID->value());
}

PyResult InsuranceService::GetInsurancePrice(PyCallArgs& call, PyInt* typeID) {
    /* called in space */
    const ItemType* type = sItemFactory.GetType(typeID->value());
    if (type != nullptr)
        return new PyFloat(type->basePrice());

    return PyStatic.NewZero();
}

InsuranceBound::InsuranceBound(EVEServiceManager& mgr, InsuranceService& parent, ShipDB* db) :
    EVEBoundObject(mgr, parent),
    m_db(db)
{
    this->Add("InsureShip", &InsuranceBound::InsureShip);
    this->Add("UnInsureShip", &InsuranceBound::UnInsureShip);
    this->Add("GetContracts", &InsuranceBound::GetContracts);
    this->Add("GetInsurancePrice", &InsuranceBound::GetInsurancePrice);

    this->m_lsc = this->GetServiceManager().Lookup <LSCService>("LSC");
}

PyResult InsuranceBound::UnInsureShip(PyCallArgs& call, PyInt* shipID) {
    m_db->DeleteInsuranceByShipID(shipID->value());
    return PyStatic.NewNone();
}

PyResult InsuranceBound::GetInsurancePrice(PyCallArgs& call, PyInt* typeID) {
    /* called when docked */
    const ItemType *type = sItemFactory.GetType(typeID->value());
    if (type != nullptr)
        return new PyFloat(type->basePrice());

    return PyStatic.NewZero();
}

PyResult InsuranceBound::GetContracts(PyCallArgs& call, std::optional<PyRep*> isCorporation) {
    if (isCorporation.has_value()) {
        return m_db->GetInsuranceByOwnerID(call.client->GetCorporationID());
    }

    return m_db->GetInsuranceByOwnerID(call.client->GetCharacterID());
}

PyResult InsuranceBound::InsureShip(PyCallArgs& call, PyInt* shipID, PyFloat* amount, std::optional<PyInt*> isCorporation) {
    call.Dump(SERVICE__CALL_DUMP);
    InventoryItemRef shipRef = sItemFactory.GetItemRef(shipID->value());
    if (shipRef.get() == nullptr)       // make error here
        return nullptr;

    if (shipRef->groupID() == EVEDB::invGroups::Rookieship) {
        call.client->SendInfoModalMsg("You cannot insure Rookie ships.");
        return PyStatic.NewNone();
    } // end rookie ship check

    /* INSURANCE FRACTION TABLE:
     *    Label    Fraction  Payment
     *   --------  --------  ------
     *   Platinum   1.0       0.3
     *   Gold       0.9       0.25
     *   Silver     0.8       0.2
     *   Bronze     0.7       0.15
     *   Standard   0.6       0.1
     *   Basic      0.5       0.05
     *   None       0.1       0.00
     */
    // calculate the fraction value
    double basePrice = shipRef->type().basePrice();
    double paymentFraction = (amount->value() / basePrice);
    if (paymentFraction < 0.025) {
        call.client->SendErrorMsg("Your payment of %.2f is below the minimum payment of %.2f required for coverage.",
                    amount->value(), (basePrice * 0.025));
        return PyStatic.NewNone();
    }

    // Map premium fraction to coverage fraction
    //   Premium% → Coverage%
    //   0.025     → 0.33  (Basic)
    //   0.05      → 0.50  (Standard)
    //   0.10      → 0.60  (Bronze)
    //   0.15      → 0.70  (Silver)
    //   0.20      → 0.80  (Gold)
    //   0.25      → 0.90  (Gold+)
    //   0.30      → 1.0   (Platinum)
    float fraction = 0.1f;
    if      (paymentFraction < 0.0375) fraction = 0.33f;
    else if (paymentFraction < 0.075)  fraction = 0.50f;
    else if (paymentFraction < 0.125)  fraction = 0.60f;
    else if (paymentFraction < 0.175)  fraction = 0.70f;
    else if (paymentFraction < 0.225)  fraction = 0.80f;
    else if (paymentFraction < 0.275)  fraction = 0.90f;
    else                               fraction = 1.0f;

    if (m_db->IsShipInsured(shipID->value())) {     //this hits db...can you insure unloaded ship? (if no, make this a ship memobj)
        if (call.byname.find("voidOld") != call.byname.end()) {
            if (call.byname.find("voidOld")->second->AsBool()->value())
                ShipDB::DeleteInsuranceByShipID(shipID->value());
        } else {  // this will send voidOld=true after asking player to cancel old insurance
            throw UserError ("InsureShipFailedSingleContract");
        }
    }

    uint8 numWeeks(12);
    double payout(shipRef->type().basePrice());
    payout *= fraction;
    if (m_db->InsertInsuranceByShipID(shipID->value(), shipRef->name(), call.client->GetCharacterID(), fraction, payout, isCorporation.has_value() && isCorporation.value()->value(), numWeeks)) {
        //  it successfully added, now, have the player pay for the insurance
        std::string reason = "Insurance Premium on ";
        reason += call.client->GetShip()->itemName();
        reason += ".  Reference ID : xxxxx";     // put contractID here

        AccountService::TransferFunds(
            call.client->GetCharacterID(),
            corpSCC,
            amount->value(),
            reason,
            Journal::EntryType::Insurance,
            -shipRef->itemID()
        );  // for paying ins, shipID should be negative
    } else {
        throw UserError ("InsureShipFailed");
    }

    // TODO:  send mail detailing insurance coverage and length of coverage
    // this works...mail is saved in db, but player not notified and cant see msg.
    const std::string subject = "New Ship Insurance"; //std::string("New application from ") + call.client->GetName(),
    std::string body = "Dear valued customer,<br>";
            body += "Congratulations on the insurance on your ship. A very wise choice indeed.<br>";
            body += "This letter is to confirm that we have issued an insurance contract for your ship, ";
            body += shipRef->itemName();
            body += ", at level of ";
            body += std::to_string(fraction * 100);
            body += "%.<br>This contract will expire after 12 weeks, on xxxxx";
            // body += GetExpireDate();   //*insert endDate Here*,
            body += "<br><br>";
            body += "Best Wishes,<br>";
            body += "The Secure Commerce Commission,<br><br>";
            body += "Reference ID: xxxxx <br>"; // put contractID here
            body += "jav";

    this->m_lsc->SendMail(corpSCC, call.client->GetCharacterID(), subject, body);

    return m_db->GetInsuranceByShipID(shipID->value());
}
