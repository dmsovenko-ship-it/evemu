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
    Author:        Allan
*/

#include "eve-server.h"

#include "account/AccountService.h"
#include "inventory/Inventory.h"
#include "station/Station.h"
#include "corporation/LPService.h"
#include "corporation/LPStore.h"

LPStore::LPStore() :
    Service("storeServer")
{
    this->Add("AcceptOffer", &LPStore::AcceptOffer);
    this->Add("GetAvailableOffers", &LPStore::GetAvailableOffers);
}

PyResult LPStore::GetAvailableOffers(PyCallArgs& call) {
    // Return all LP store offers for the station's NPC corporation
    // Format: list of {offerID, typeID, iskCost, lpCost, qty, reqItems}
    uint32 corpID = call.client->GetCorporationID();

    // Try station owner NPC corp for LP store context
    uint32 stationID = call.client->GetStationID();
    if (stationID > 0) {
        StationItemRef station = sItemFactory.GetStationRef(stationID);
        if (station)
            corpID = station->GetOwnerID();
    }

    DBQueryResult dbRes;
    if (!sDatabase.RunQuery(dbRes,
        "SELECT typeID, iskCost, storeID, quantity, lpCost"
        " FROM lpStore WHERE corporationID = %u", corpID))
        return new PyList();

    PyList* result = new PyList();
    DBResultRow row;
    while (dbRes.GetRow(row)) {
        DBQueryResult reqRes;
        sDatabase.RunQuery(reqRes,
            "SELECT typeID, quantity FROM lpRequiredItems WHERE parentID = %u", row.GetInt(2));

        PyList* reqList = new PyList();
        DBResultRow reqRow;
        while (reqRes.GetRow(reqRow)) {
            PyTuple* req = new PyTuple(2);
            req->SetItemInt(0, reqRow.GetInt(0));
            req->SetItemInt(1, reqRow.GetInt(1));
            reqList->AddItem(req);
        }

        PyDict* dict = new PyDict();
        dict->SetItem("offerID", new PyInt(row.GetInt(2)));
        dict->SetItem("typeID", new PyInt(row.GetInt(0)));
        dict->SetItem("iskCost", new PyInt(row.GetInt(1)));
        dict->SetItem("lpCost", new PyInt(row.GetInt(4)));
        dict->SetItem("qty", new PyInt(row.GetInt(3)));
        dict->SetItem("reqItems", reqList);
        result->AddItem(new PyObject("util.KeyVal", dict));
    }
    return result;
}

PyResult LPStore::AcceptOffer(PyCallArgs& call, PyInt* offerID, PyInt* quantity) {
    // Purchase an LP store offer
    uint32 storeID = offerID->value();
    uint32 charID = call.client->GetCharacterID();

    // Lookup offer
    DBQueryResult offerRes;
    if (!sDatabase.RunQuery(offerRes,
        "SELECT typeID, iskCost, quantity, lpCost, corporationID"
        " FROM lpStore WHERE storeID = %u LIMIT 1", storeID))
        return new PyNone();

    DBResultRow offerRow;
    if (!offerRes.GetRow(offerRow))
        return new PyNone();

    uint32 typeID = offerRow.GetInt(0);
    uint32 iskCost = offerRow.GetInt(1);
    uint32 qty = offerRow.GetInt(2);
    uint32 lpCost = offerRow.GetInt(3);
    uint32 corpID = offerRow.GetInt(4);

    // Check LP balance
    DBQueryResult lpRes;
    sDatabase.RunQuery(lpRes,
        "SELECT balance FROM lpWallet"
        " WHERE characterID = %u AND corporationID = %u LIMIT 1",
        charID, corpID);
    DBResultRow lpRow;
    int32 lpBalance = 0;
    if (lpRes.GetRow(lpRow))
        lpBalance = lpRow.GetInt(0);
    if (lpBalance < (int32)lpCost) {
        call.client->SendErrorMsg("Not enough LP.");
        return new PyNone();
    }

    // Check ISK
    if (call.client->GetBalance() < iskCost) {
        call.client->SendErrorMsg("Not enough ISK.");
        return new PyNone();
    }

    // Check required items
    DBQueryResult reqRes;
    sDatabase.RunQuery(reqRes,
        "SELECT typeID, quantity FROM lpRequiredItems WHERE parentID = %u", storeID);
    DBResultRow reqRow;
    while (reqRes.GetRow(reqRow)) {
        uint32 reqTypeID = reqRow.GetUInt(0);
        uint32 reqQty = reqRow.GetUInt(1);
        int32 entityID = sItemFactory.GetStationRef(call.client->GetStationID())
            ->GetMyInventory()->ContainsTypeStackQtyByFlag(reqTypeID, flagHangar, reqQty);
        if (entityID == 0) {
            call.client->SendErrorMsg("Required items are missing.");
            return new PyNone();
        }
        InventoryItemRef itemRef = sItemFactory.GetItemRef(entityID);
        if (itemRef and itemRef->quantity() > reqQty)
            itemRef->AlterQuantity(-(int32)reqQty);
        else if (itemRef)
            itemRef->Delete();
    }

    // Deduct LP & ISK
    LPService::AddLP(charID, corpID, -(int32)lpCost);
    AccountService::TransferFunds(charID, corpID, iskCost,
        "LP Store purchase", Journal::EntryType::PaymentToLPStore, storeID);

    // Spawn item
    ItemData iData(typeID, charID, call.client->GetStationID(), flagHangar, qty);
    InventoryItemRef iRef = sItemFactory.SpawnItem(iData);
    if (iRef) {
        iRef->Move(call.client->GetStationID(), flagHangar, true);
        iRef->SaveItem();
    }

    return new PyNone();
}

