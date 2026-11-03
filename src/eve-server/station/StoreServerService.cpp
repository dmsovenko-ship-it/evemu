/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.
    ------------------------------------------------------------------------------------
    Author:        allan (storeServer for the CQ NeX store)
*/

#include "eve-server.h"

#include "station/StoreServerService.h"
#include "account/AccountService.h"

/*
 *  Client contract (script:/ui/services/storeSvc.py + ui/station/virtualGoodsStore.py):
 *
 *    GetAvailableOffers() -> dict {offerID: KeyVal{ goods: PyList }} where every
 *    goods item is a KeyVal with:
 *        offered         bool    - true for the product, false for the price
 *        argumentType    int     - storeConst.GOODTYPE_EVETYPEID(2) for product,
 *                                  storeConst.GOODTYPE_CREDITS(1) for price
 *        argumentValue   int     - typeID for product, credit type for price
 *        amount          int     - quantity / price
 *        details1..3     int     - bpME / bpPE / bpRuns (0 for non-BP offers)
 *
 *    The client ERRORS (offer marked corrupt and skipped) only when the price
 *    argumentValue == const.creditsISK, and silently IGNORES any credit value
 *    that is neither ISK nor AURUM.  Our decompile does not contain the client
 *    `const` module, so the exact creditsAURUM number is unverified -- we send
 *    BOTH candidate values (2 and 3) with the same price.  Whichever the client
 *    treats as AURUM is summed once; the other is ignored.  Never send 1.
 */

const uint32 NEX_PAYEE_ID = 1000001;   // CONCORD NPC corp - TransferFunds does nothing on this side (payee sink)

StoreServerService::StoreServerService()
: Service("storeServer", eAccessLevel_Character)
{
    this->Add("GetAvailableOffers", &StoreServerService::GetAvailableOffers);
    this->Add("AcceptOffer", &StoreServerService::AcceptOffer);
}

uint32 StoreServerService::PriceForType(uint32 typeID)
{
    // deterministic per-type price in AUR, stable across boots
    return 500 + (typeID % 24) * 250;
}

PyRep* StoreServerService::BuildGoodsList(uint32 typeID, uint32 quantity, uint32 price)
{
    PyList* goods = new PyList();
    {
        // the offered product
        PyDict* d = new PyDict();
            d->SetItemString("offered", new PyBool(true));
            d->SetItemString("argumentType", new PyInt(2));     // GOODTYPE_EVETYPEID
            d->SetItemString("argumentValue", new PyInt(typeID));
            d->SetItemString("amount", new PyInt(quantity));
            d->SetItemString("details1", new PyInt(0));         // bpME
            d->SetItemString("details2", new PyInt(0));         // bpPE
            d->SetItemString("details3", new PyInt(0));         // bpRuns
        goods->AddItem(new PyObject("util.KeyVal", d));
    }
    for (int creditVal = 2; creditVal <= 3; ++creditVal) {
        // price candidates - see the comment at the top (client ignores non-AURUM)
        PyDict* d = new PyDict();
            d->SetItemString("offered", new PyBool(false));
            d->SetItemString("argumentType", new PyInt(1));     // GOODTYPE_CREDITS
            d->SetItemString("argumentValue", new PyInt(creditVal));
            d->SetItemString("amount", new PyInt(price));
        goods->AddItem(new PyObject("util.KeyVal", d));
    }
    PyDict* offer = new PyDict();
        offer->SetItemString("goods", goods);
    return new PyObject("util.KeyVal", offer);
}

PyResult StoreServerService::GetAvailableOffers(PyCallArgs& call)
{
    _log(SERVICE__MESSAGE, "StoreServerService::Handle_GetAvailableOffers()", "size=%lu", call.tuple->size());

    // full published Apparel catalog (category 30) - the client filters/labels it itself
    PyDict* offers = new PyDict();
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT t.typeID FROM invTypes t JOIN invGroups g ON g.groupID = t.groupID"
        " WHERE g.categoryID = 30 AND t.published = 1 ORDER BY t.typeID"))
    {
        DBResultRow row;
        uint32 offerID = 0;
        while (res.GetRow(row)) {
            ++offerID;
            uint32 typeID = row.GetUInt(0);
            uint32 price = PriceForType(typeID);
            PyDict* offer = new PyDict();
                offer->SetItemString("goods", BuildGoodsList(typeID, 1, price));
            offers->SetItem(new PyInt(offerID), new PyObject("util.KeyVal", offer));
        }
    }

    _log(SERVICE__MESSAGE, "StoreServerService::Handle_GetAvailableOffers()", "returning %u offers", offers->size());
    return offers;
}

PyResult StoreServerService::AcceptOffer(PyCallArgs& call)
{
    _log(SERVICE__MESSAGE, "StoreServerService::Handle_AcceptOffer()", "size=%lu", call.tuple->size());

    if (call.tuple->size() < 2)
        throw UserError("VStoreNotAvailable");

    PyRep* offerRep = call.tuple->GetItem(0);
    PyRep* qtyRep = call.tuple->GetItem(1);
    if (offerRep == nullptr || qtyRep == nullptr)
        throw UserError("VStoreNotAvailable");

    uint32 offerID = offerRep->IsInt() ? (uint32)(offerRep->AsInt()->value()) : 0;
    uint32 quantity = qtyRep->IsInt() ? (uint32)(qtyRep->AsInt()->value()) : 0;
    if (offerID == 0 || quantity == 0 || quantity > 10)
        throw UserError("VStoreNotAvailable");

    Client* pClient = call.client;
    if (pClient == nullptr || !pClient->IsDock() || pClient->GetStationID() == 0)
        throw UserError("VStoreNotAvailable");

    // resolve the offer back to a typeID from the same deterministic catalog
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT t.typeID FROM invTypes t JOIN invGroups g ON g.groupID = t.groupID"
        " WHERE g.categoryID = 30 AND t.published = 1 ORDER BY t.typeID LIMIT %u, 1", offerID - 1))
        throw UserError("VStoreNotAvailable");

    DBResultRow row;
    if (!res.GetRow(row))
        throw UserError("OfferNotAvailableNow");     // client invalidates its offer cache on this one

    uint32 typeID = row.GetUInt(0);
    uint32 price = PriceForType(typeID) * quantity;

    double balance = pClient->GetBalance(Account::CreditType::AURUM);
    if (balance < (double)price)
        throw UserError("NotEnoughCredits");

    std::string typeName = "item";
    DBQueryResult nres;
    if (sDatabase.RunQuery(nres, "SELECT typeName FROM invTypes WHERE typeID = %u", typeID)) {
        DBResultRow nrow;
        if (nres.GetRow(nrow) && nrow.GetText(0) != nullptr)
            typeName = nrow.GetText(0);
    }
    std::string reason = "NeX Store: ";
    reason += typeName;
    reason += " x";
    reason += std::to_string(quantity);

    // charge the AUR wallet (journal entry 106 = StorePurchase; NPC payee sink)
    AccountService::TransferFunds(
        pClient->GetCharacterID(),
        NEX_PAYEE_ID,
        (double)price,
        reason,
        Journal::EntryType::StorePurchase,
        typeID,
        Account::KeyType::AUR,
        Account::KeyType::Cash,
        pClient
    );

    // deliver into the station hangar
    ItemData idata(typeID, pClient->GetCharacterID(), pClient->GetStationID(), flagHangar, quantity);
    InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
    if (iRef.get() == nullptr)
        throw UserError("VStoreNotAvailable");
    iRef->Move(pClient->GetStationID(), flagHangar, true);

    pClient->SendNotifyMsg("NeX Store: %s x%u delivered to your hangar.", typeName.c_str(), quantity);
    return new PyNone();
}
