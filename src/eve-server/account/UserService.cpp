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
    Author:        Bloody.Rabbit
*/

#include "eve-server.h"


#include "account/UserService.h"
#include "inventory/ItemFactory.h"
#include "inventory/InventoryItem.h"
#include "services/ServiceManager.h"

MovementService::MovementService(EVEServiceManager *mgr) :
    Service("movementServer"),
    m_manager(mgr)
{
    this->Add("ResolveNodeID", &MovementService::ResolveNodeID);
}

PyResult MovementService::ResolveNodeID(PyCallArgs& call, PyInt* newWorldSpaceId)
{
    sLog.Yellow( "MovementService", "ResolveNodeID" );
    call.Dump(CHARACTER__DEBUG);

    return new PyInt(this->m_manager->GetNodeID());
}

UserService::UserService() :
    Service("userSvc", eAccessLevel_User)
{
    this->Add("GetRedeemTokens", &UserService::GetRedeemTokens);
    this->Add("ClaimRedeemTokens", &UserService::ClaimRedeemTokens);
    this->Add("ReverseRedeem", &UserService::ReverseRedeem);
    this->Add("GetCreateDate", &UserService::GetCreateDate);
    this->Add("ReportISKSpammer", &UserService::ReportISKSpammer);
    this->Add("ReportBot", &UserService::ReportBot);
    this->Add("ApplyPilotLicence", &UserService::ApplyPilotLicence);
}

PyResult UserService::GetRedeemTokens(PyCallArgs& call)
{
    Client* pClient = call.client;
    if (pClient == nullptr)
        return new PyList();

    // vitrine RedeemPlex: PLEX owned by the character in their current station
    // hangar are presented as redeem tokens; ClaimRedeemTokens delivers them
    // to the target character's hangar.
    PyList* list = new PyList();
    DBQueryResult res;
    uint32 charID = pClient->GetCharacterID();
    uint32 stationID = pClient->GetStationID();
    if (charID != 0 && stationID != 0) {
        if (!sDatabase.RunQuery(res,
            "SELECT itemID, quantity FROM entity WHERE ownerID = %u AND typeID = 29668"
            " AND flag = 211 AND locationID = %u",
            charID, stationID)) {
            sLog.Error("GetRedeemTokens", "query failed: %s", res.error.c_str());
            return list;
        }
    } else {
        // login race: the client fetches tokens before session charid/stationid
        // are flushed to the Client - fall back to all chars of the account
        uint32 accountID = (uint32)pClient->GetUserID();
        sLog.Error("GetRedeemTokens", "login race (char=%u station=%u) - fallback by accountID=%u", charID, stationID, accountID);
        if (accountID == 0 || !sDatabase.RunQuery(res,
            "SELECT e.itemID, e.quantity FROM entity e JOIN chrCharacters c ON c.characterID = e.ownerID"
            " WHERE c.accountID = %u AND e.typeID = 29668 AND e.flag = 211",
            accountID)) {
            sLog.Error("GetRedeemTokens", "fallback query failed: %s", res.error.c_str());
            return list;
        }
    }
    DBResultRow row;
    uint32 cnt = 0;
    while (res.GetRow(row)) {
        ++cnt;
        PyDict* d = new PyDict();
            d->SetItemString("tokenID", new PyInt(row.GetUInt(0)));
            d->SetItemString("massTokenID", new PyInt(0));
            d->SetItemString("typeID", new PyInt(29668));
            d->SetItemString("quantity", new PyInt(row.GetUInt(1)));
            d->SetItemString("description", new PyString("PLEX"));
            d->SetItemString("label", new PyString(""));
            d->SetItemString("stationID", new PyInt(stationID != 0 ? stationID : row.GetUInt(2)));
            d->SetItemString("expireDateTime", new PyInt(0));
        list->AddItem(new PyObject("util.KeyVal", d));
    }
    sLog.Error("GetRedeemTokens", "char=%u station=%u -> %u tokens", charID, stationID, cnt);
    return list;
}

PyResult UserService::ClaimRedeemTokens(PyCallArgs& call, PyList* tokens, PyInt* charIDIn)
{
    Client* pClient = call.client;
    if (pClient == nullptr || tokens == nullptr || charIDIn == nullptr)
        return new PyNone();
    uint32 targetCharID = (uint32)charIDIn->value();
    uint32 callerCharID = (uint32)pClient->GetCharacterID();
    if (callerCharID == 0)
        callerCharID = targetCharID;    // claim from charsel: session charid not set yet
    uint32 accountID = (uint32)pClient->GetUserID();
    sLog.Error("ClaimRedeemTokens", "target=%u caller=%u account=%u tokens=%u", targetCharID, callerCharID, accountID, tokens->size());
    // target char must belong to the caller's account (charsel sends charID from its own UI)
    DBQueryResult vres;
    if (!sDatabase.RunQuery(vres,
        "SELECT 1 FROM chrCharacters WHERE characterID = %u AND accountID = %u",
        targetCharID, accountID) || vres.GetRowCount() == 0) {
        sLog.Error("ClaimRedeemTokens", "target char %u is not owned by account %u - rejected", targetCharID, accountID);
        throw UserError("RedeemNoTokens");
    }

    int64 nowft = GetFileTimeNow();
    for (size_t i = 0; i < tokens->size(); ++i) {
        PyRep* r = tokens->GetItem(i);
        if (r == nullptr) continue;
        uint32 itemID = 0;
        if ((r)->IsTuple()) {
            PyTuple* pair = (r)->AsTuple();
            if (pair->size() >= 1 && (pair->GetItem(0)) != nullptr && (pair->GetItem(0))->IsInt())
                itemID = (uint32)(pair->GetItem(0))->AsInt()->value();
        } else if ((r)->IsInt()) {
            itemID = (uint32)(r)->AsInt()->value();
        } else if ((r)->IsDict()) {
            PyDict* d = (r)->AsDict();
            PyRep* key = d->GetItemString("tokenID");
            if (key != nullptr && (key)->IsInt())
                itemID = (uint32)(key)->AsInt()->value();
        }
        if (itemID == 0) {
            sLog.Error("ClaimRedeemTokens", "token %u: unparseable type=%s - skipped", i, (r)->TypeString());
            continue;
        }
        InventoryItemRef plex = sItemFactory.GetItemRef(itemID);
        if (plex.get() == nullptr) {
            sLog.Error("ClaimRedeemTokens", "token %u: item %u not found", i, itemID);
            continue;
        }
        if (plex->typeID() != 29668) {
            sLog.Error("ClaimRedeemTokens", "token %u: item %u is type %u (not PLEX)", i, itemID, plex->typeID());
            continue;
        }
        // source PLEX must belong to a char of the caller's account (redeem queue)
        DBQueryResult ores;
        if (!sDatabase.RunQuery(ores,
            "SELECT 1 FROM chrCharacters WHERE characterID = %u AND accountID = %u",
            plex->ownerID(), accountID) || ores.GetRowCount() == 0) {
            sLog.Error("ClaimRedeemTokens", "token %u: item %u owner %u not in account %u - skipped", i, itemID, plex->ownerID(), accountID);
            continue;
        }
        // deliver to the station where the queue item sits (works from charsel too,
        // where the caller has no stationID yet)
        uint32 stationID = plex->locationID();
        sLog.Error("ClaimRedeemTokens", "token %u: claiming item %u (%u x PLEX) -> char %u hangar @ %u", i, itemID, plex->quantity(), targetCharID, stationID);
        plex->ChangeOwner(targetCharID, false);
        plex->Move(stationID, flagHangar, true);
        sLog.Error("ClaimRedeemTokens", "token %u: done - item %u now loc=%u flag=%u", i, itemID, plex->locationID(), plex->flag());
        (void)nowft;
    }
    return new PyNone();
}

PyResult UserService::ReverseRedeem(PyCallArgs& call, PyInt* itemID)
{
    //sm.RemoteSvc('userSvc').ReverseRedeem(item.itemID)
    sLog.Yellow( "UserService", "Handle_ReverseRedeem" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
    
    /* {'messageKey': 'ReverseRedeemingCompleted', 'dataID': 17877681, 'suppressable': False, 'bodyID': 257344, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257343, 'messageID': 2917}
     * {'messageKey': 'ReverseRedeemingFatalError', 'dataID': 17877443, 'suppressable': False, 'bodyID': 257253, 'messageType': 'error', 'urlAudio': '', 'urlIcon': '', 'titleID': 257252, 'messageID': 2918}
     * {'messageKey': 'ReverseRedeemingIllegalType', 'dataID': 17877511, 'suppressable': False, 'bodyID': 257279, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257278, 'messageID': 2913}
     * {'messageKey': 'ReverseRedeemingOnlyInStation', 'dataID': 17877516, 'suppressable': False, 'bodyID': 257281, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257280, 'messageID': 2911}
     * {'messageKey': 'ReverseRedeemingYouDoNotOwnItem', 'dataID': 17877433, 'suppressable': False, 'bodyID': 257249, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257248, 'messageID': 2914}
     */
}

PyResult UserService::GetCreateDate(PyCallArgs& call)
{
    return new PyLong(call.client->GetChar()->createDateTime());
}

PyResult UserService::ReportISKSpammer(PyCallArgs& call, PyInt* characterID, PyInt* channelID)
{
    // sm.RemoteSvc('userSvc').ReportISKSpammer(charID, channelID, spamEntries)
    sLog.Yellow( "UserService", "Handle_ReportISKSpammer" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
}

PyResult UserService::ReportBot(PyCallArgs& call, PyInt* itemID)
{
    sLog.Yellow( "UserService", "Handle_ReportBot" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
}

PyResult UserService::ApplyPilotLicence(PyCallArgs& call, PyInt* itemID)
{
    sLog.Yellow("UserService", "Handle_ApplyPilotLicence");
    call.Dump(CHARACTER__DEBUG);

    uint32 plexID = (itemID != nullptr) ? (uint32)itemID->value() : 0;
    if (plexID == 0)
        return nullptr;

    Client* client = call.client;
    if (client == nullptr)
        return nullptr;
    uint32 charID = client->GetCharacterID();
    if (charID == 0)
        return nullptr;

    // The PLEX must be a real item owned by this character (hangar flag 4 or cargo flag 5)
    uint32 plexQty = 1;
    DBQueryResult pres;
    if (!sDatabase.RunQuery(pres,
            "SELECT quantity FROM entity WHERE itemID=%u AND ownerID=%u AND typeID=29668 AND quantity>0 LIMIT 1",
            plexID, charID))
        return nullptr;
    DBResultRow prow;
    if (!pres.GetRow(prow))
        return nullptr;   // not a PLEX / not ours
    plexQty = prow.GetUInt(0);

    // Resolve the account
    uint32 accountID = 0;
    DBQueryResult ares;
    if (sDatabase.RunQuery(ares, "SELECT accountID FROM chrCharacters WHERE characterID=%u", charID)) {
        DBResultRow arow;
        if (ares.GetRow(arow))
            accountID = arow.GetUInt(0);
    }
    if (accountID == 0)
        return nullptr;

    // Consume ONE PLEX from the stack (a stack of N is a single entity row).
    // In-memory op first (removes from container + emits the inventory update
    // the client needs), then SQL as a safety net if the item was not loaded.
    InventoryItemRef plexRef = sItemFactory.GetItemRef(plexID);
    if (plexRef.get() != nullptr) {
        if (plexQty > 1 && plexRef->quantity() > 1)
            plexRef->AlterQuantity(-1, true);
        else
            plexRef->Delete();
    }
    DBerror err;
    if (plexQty > 1) {
        sDatabase.RunQuery(err, "UPDATE entity SET quantity=quantity-1 WHERE itemID=%u AND quantity>1", plexID);
    } else {
        sDatabase.RunQuery(err, "DELETE FROM entity WHERE itemID=%u", plexID);
        sDatabase.RunQuery(err, "DELETE FROM entity_attributes WHERE itemID=%u", plexID);
    }

    // Extend the subscription (same math as the portal's char/ActivatePlex)
    uint32 days = (sConfig.subscription.PremiumDays > 0 ? sConfig.subscription.PremiumDays : 30);
    int64 add = (int64)days * 86400LL * 10000000LL;
    sDatabase.RunQuery(err,
        "UPDATE account SET premiumUntil = GREATEST(premiumUntil, (UNIX_TIMESTAMP()+11644473600)*10000000) + %lld WHERE accountID=%u",
        add, accountID);

    // The alpha flag cache in Character.cpp re-reads premiumUntil on a 60s TTL.

    client->SendNotifyMsg("30 Day Pilot's License Extension activated - account is now Omega for %u days.", days);
    return new PyInt((int)days);
}
