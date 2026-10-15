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

#ifndef STORE_SERVER_SERVICE_H
#define STORE_SERVER_SERVICE_H

#include "services/Service.h"

/**
 * \brief The CQ NeX (virtual goods) store service.
 *
 * The client's store window calls storeServer.GetAvailableOffers() and renders
 * each offer as {offerID: KeyVal{ goods: [product, price...] }}.  Buying goes
 * through storeServer.AcceptOffer(offerID, quantity): we charge the character's
 * AUR wallet and deliver the item into the station hangar.
 */
class StoreServerService : public Service<StoreServerService>
{
public:
    StoreServerService();
    virtual ~StoreServerService() = default;

protected:
    PyResult GetAvailableOffers(PyCallArgs& call);
    PyResult AcceptOffer(PyCallArgs& call);

private:
    static uint32 PriceForType(uint32 typeID);
    static PyRep* BuildGoodsList(uint32 typeID, uint32 quantity, uint32 price);
};

#endif  // STORE_SERVER_SERVICE_H
