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
    Updates:    Allan
*/

// this class isnt a singleton, but is instantiated once on server start.


#include "StaticDataMgr.h"
#include "account/AccountService.h"
#include "cache/ObjCacheService.h"
#include "inventory/Inventory.h"
#include "market/MarketMgr.h"
#include "market/MarketProxyService.h"
#include "station/StationDataMgr.h"
#include "system/SystemManager.h"
#include "standing/StandingDB.h"

/*
 * MARKET__ERROR
 * MARKET__WARNING
 * MARKET__MESSAGE
 * MARKET__DEBUG
 * MARKET__TRACE
 * MARKET__DB_ERROR
 * MARKET__DB_TRACE
 */

MarketProxyService::MarketProxyService(EVEServiceManager& mgr) :
    Service("marketProxy"),
    m_manager (mgr)
{
    m_cache = this->m_manager.Lookup <ObjCacheService>("objectCaching");

    this->Add("StartupCheck", &MarketProxyService::StartupCheck);
    this->Add("GetStationAsks", &MarketProxyService::GetStationAsks);
    this->Add("GetSystemAsks", &MarketProxyService::GetSystemAsks);
    this->Add("GetRegionBest", &MarketProxyService::GetRegionBest);
    this->Add("GetMarketGroups", &MarketProxyService::GetMarketGroups);
    this->Add("GetOrders", &MarketProxyService::GetOrders);
    this->Add("GetOldPriceHistory", &MarketProxyService::GetOldPriceHistory);
    this->Add("GetNewPriceHistory", &MarketProxyService::GetNewPriceHistory);
    this->Add("PlaceCharOrder", static_cast <PyResult (MarketProxyService::*)(PyCallArgs &call, PyInt*, PyInt*, PyFloat*, PyInt*, PyInt*, PyInt*, std::optional <PyInt*>, PyInt*, PyInt*, PyBool*, std::optional<PyRep*>)> (&MarketProxyService::PlaceCharOrder));
    this->Add("PlaceCharOrder", static_cast <PyResult (MarketProxyService::*)(PyCallArgs &call, PyInt*, PyInt*, PyFloat*, PyInt*, PyInt*, PyInt*, std::optional <PyInt*>, PyInt*, PyInt*, PyInt*, std::optional<PyRep*>)> (&MarketProxyService::PlaceCharOrder));
    this->Add("GetCharOrders", &MarketProxyService::GetCharOrders);
    this->Add("GetSkillLimits", &MarketProxyService::GetSkillLimits);
    this->Add("ModifyCharOrder", &MarketProxyService::ModifyCharOrder);
    this->Add("CancelCharOrder", &MarketProxyService::CancelCharOrder);
    this->Add("CharGetNewTransactions", &MarketProxyService::CharGetNewTransactions);
    this->Add("CharGetTransactions", &MarketProxyService::CharGetTransactions);
    this->Add("CorpGetNewTransactions", &MarketProxyService::CorpGetNewTransactions);
    this->Add("CorpGetTransactions", &MarketProxyService::CorpGetTransactions);
    this->Add("GetSkillLimits", &MarketProxyService::GetSkillLimits);
    this->Add("GetCharSkillLimits", &MarketProxyService::GetSkillLimits);  // same method, alias for client cache
    this->Add("GetCorporationOrders", &MarketProxyService::GetCorporationOrders);
}

PyResult MarketProxyService::GetMarketGroups(PyCallArgs &call) {
    return sMktMgr.GetMarketGroups();
}

PyResult MarketProxyService::StartupCheck(PyCallArgs &call) {
    //if (sMktMgr.NeedsUpdate())
    //    sMktMgr.UpdatePriceHistory();
    return nullptr;
}

PyResult MarketProxyService::GetCharOrders(PyCallArgs &call) {
    return MarketDB::GetOrdersForOwner(call.client->GetCharacterID());
}

PyResult MarketProxyService::GetCorporationOrders(PyCallArgs &call) {
    return MarketDB::GetOrdersForOwner(call.client->GetCorporationID());
}

// station, system, region based on selection in market window
PyResult MarketProxyService::GetStationAsks(PyCallArgs &call) {
    return sMktMgr.GetStationAsks(call.client->GetStationID());
}

PyResult MarketProxyService::GetSystemAsks(PyCallArgs &call) {
    return sMktMgr.GetSystemAsks(call.client->GetSystemID());
}

PyResult MarketProxyService::GetRegionBest(PyCallArgs &call) {
    return sMktMgr.GetRegionBest(call.client->GetRegionID());
}

// this is called 3x on every market transaction
PyResult MarketProxyService::GetOldPriceHistory(PyCallArgs &call, PyInt* typeID) {
    return sMktMgr.GetOldPriceHistory(call.client->GetRegionID(), typeID->value());
}

PyResult MarketProxyService::GetNewPriceHistory(PyCallArgs& call, PyInt* typeID) {
    return sMktMgr.GetNewPriceHistory(call.client->GetRegionID(), typeID->value());
}

PyResult MarketProxyService::CharGetNewTransactions(PyCallArgs &call, PyRep* sellBuy, PyRep* typeID, PyRep* clientID, PyRep* quantity, PyRep* fromDate, PyRep* maxPrice, PyRep* minPrice) {
    Market::TxData data = Market::TxData();

    data.clientID = clientID->IsNone() ? 0 : PyRep::IntegerValueU32(clientID);
    data.isBuy = sellBuy->IsNone() ? -1 : PyRep::IntegerValueU32(sellBuy);
    data.price = minPrice->IsNone() ? 0 : PyRep::IntegerValueU32(minPrice);
    data.quantity = quantity->IsNone() ? 0 : PyRep::IntegerValueU32(quantity);
    data.typeID = typeID->IsNone() ? 0 : PyRep::IntegerValueU32(typeID);
    data.time = fromDate->IsNone() ? 0 : PyRep::IntegerValue(fromDate);
    data.accountKey = Account::KeyType::Cash;

    return MarketDB::GetTransactions(call.client->GetCharacterID(), data);
}

PyResult MarketProxyService::CorpGetNewTransactions(PyCallArgs& call, PyRep* sellBuy, PyRep* typeID, PyRep* clientID, PyRep* quantity, PyRep* fromDate, PyRep* maxPrice, PyRep* minPrice, PyRep* accountKey, PyRep* memberID)
{
    Market::TxData data = Market::TxData();
        data.clientID = clientID->IsNone() ? 0 : PyRep::IntegerValueU32(clientID);
        data.isBuy = sellBuy->IsNone() ? 0 : PyRep::IntegerValueU32(sellBuy);
        data.price = minPrice->IsNone() ? 0 : PyRep::IntegerValueU32(minPrice);
        data.quantity = quantity->IsNone() ? 0 : PyRep::IntegerValueU32(quantity);
        data.typeID = typeID->IsNone() ? 0 : PyRep::IntegerValueU32(typeID);
        data.time = fromDate->IsNone() ? 0 : PyRep::IntegerValue(fromDate);
        data.accountKey = accountKey->IsNone() ? 0 : PyRep::IntegerValueU32(accountKey);
        data.memberID = memberID->IsNone() ? 0 : PyRep::IntegerValueU32(memberID);
    return MarketDB::GetTransactions(call.client->GetCorporationID(), data);
}

PyResult MarketProxyService::CorpGetTransactions(PyCallArgs& call, PyRep* lastTransactionID, PyRep* sellBuy, PyRep* typeID, PyRep* clientID, PyRep* quantity, PyRep* fromDate, PyRep* maxPrice, PyRep* minPrice, PyRep* accountKey, PyRep* memberID)
{
    return CorpGetNewTransactions(call, sellBuy, typeID, clientID, quantity, fromDate, maxPrice, minPrice, accountKey, memberID);
}

PyResult MarketProxyService::CharGetTransactions(PyCallArgs& call, PyRep* lastTransactionID, PyRep* sellBuy, PyRep* typeID, PyRep* clientID, PyRep* quantity, PyRep* fromDate, PyRep* maxPrice, PyRep* minPrice)
{
    return CharGetNewTransactions(call, sellBuy, typeID, clientID, quantity, fromDate, maxPrice, minPrice);
}

PyResult MarketProxyService::GetSkillLimits(PyCallArgs& call) {
    _log(MARKET__MESSAGE, "MarketProxyService::GetSkillLimits() size=%lli", call.tuple->size());

    CharacterRef pChar = call.client->GetChar();
    if (pChar.get() == nullptr)
        return PyStatic.NewNone();

    int8 tradeLvl       = pChar->GetSkillLevel(EvESkill::Trade);
    int8 retailLvl      = pChar->GetSkillLevel(EvESkill::Retail);
    int8 wholeSaleLvl   = pChar->GetSkillLevel(EvESkill::Wholesale);
    int8 tycoonLvl      = pChar->GetSkillLevel(EvESkill::Tycoon);
    int8 accountingLvl  = pChar->GetSkillLevel(EvESkill::Accounting);
    int8 brokerLvl      = pChar->GetSkillLevel(EvESkill::BrokerRelations);
    int8 marginTradeLvl = pChar->GetSkillLevel(EvESkill::MarginTrading);
    int8 marketingLvl   = std::min<int8>(pChar->GetSkillLevel(EvESkill::Marketing), 5);
    int8 procurementLvl = std::min<int8>(pChar->GetSkillLevel(EvESkill::Procurement), 5);
    int8 visibilityLvl  = std::min<int8>(pChar->GetSkillLevel(EvESkill::Visibility), 5);
    int8 daytradingLvl  = std::min<int8>(pChar->GetSkillLevel(EvESkill::Daytrading), 5);

    int32 maxOrderCount = 5 + tradeLvl * 4 + retailLvl * 8 + wholeSaleLvl * 16 + tycoonLvl * 32;

    float commissionPct = 0.01f;
    commissionPct *= (1.0f - brokerLvl * 0.05f);
    float transactionTax = 0.015f;
    transactionTax *= (1.0f - accountingLvl * 0.1f);

    static const int jumpsPerSkillLevel[] = { 0, 5, 10, 20, 40, 80 };

    PyDict* limits = new PyDict();
    limits->SetItemString("cnt", new PyInt(maxOrderCount));
    limits->SetItemString("fee", new PyFloat(commissionPct));
    limits->SetItemString("acc", new PyFloat(transactionTax));
    limits->SetItemString("ask", new PyInt(jumpsPerSkillLevel[marketingLvl]));
    limits->SetItemString("bid", new PyInt(jumpsPerSkillLevel[procurementLvl]));
    limits->SetItemString("vis", new PyInt(jumpsPerSkillLevel[visibilityLvl]));
    limits->SetItemString("mod", new PyInt(jumpsPerSkillLevel[daytradingLvl]));
    limits->SetItemString("esc", new PyFloat(std::pow(0.75f, marginTradeLvl)));
    return limits;
}

PyResult MarketProxyService::GetOrders(PyCallArgs &call, PyInt* typeID) {
    PyRep* result(nullptr);
    std::string method_name ("GetOrders_");
    method_name += std::to_string(call.client->GetRegionID());
    method_name += "_";
    method_name += std::to_string(typeID->value());
    ObjectCachedMethodID method_id(GetName().c_str(), method_name.c_str());
    //check to see if this method is in the cache already.
    if (!this->m_cache->IsCacheLoaded(method_id))
    {
        //this method is not in cache yet, load up the contents and cache it.
        result = MarketDB::GetOrders(call.client->GetRegionID(), typeID->value());
        if (result == nullptr) {
            _log(MARKET__DB_ERROR, "Failed to load cache, generating empty contents.");
            result = PyStatic.NewNone();
        }
        this->m_cache->GiveCache(method_id, &result);
    }

    //now we know its in the cache one way or the other, so build a
    //cached object cached method call result.
    result = this->m_cache->MakeObjectCachedMethodCallResult(method_id);

    /*{'FullPath': u'UI/Messages', 'messageID': 258616, 'label': u'MktMarketOpeningTitle'}(u'Market not open yet', None, None)
     * {'FullPath': u'UI/Messages', 'messageID': 258617, 'label': u'MktMarketOpeningBody'}
     * (u'{region} market region is currently opening up for business and is not yet ready.
     * Please try again in a few minutes, or refer to another market region for your trading needs.',
     * None, {u'{region}': {'conditionalValues': [], 'variableType': 10, 'propertyName': None, 'args': 0, 'kwargs': {}, 'variableName': 'region'}})
     */
    return result;
}

PyResult MarketProxyService::PlaceCharOrder(PyCallArgs &call, PyInt* stationID, PyInt* typeID, PyFloat* price, PyInt* quantity, PyInt* bid, PyInt* orderRange, std::optional <PyInt*> itemID, PyInt* minVolume, PyInt* duration, PyBool* useCorp, std::optional<PyRep*> located) {
    //self.GetMarketProxy().PlaceCharOrder(int(stationID), int(typeID), round(float(price), 2), int(quantity), int(bid), int(orderRange),
    //   itemID = None, int(minVolume = 1), int(duration = 14), useCorp = False, located = None)
    // located = [officeFolderID, officeID] or None

    /*
    21:47:34 [MktDump] Mkt::PlaceCharOrder()
    21:47:34 [MktDump]     Call_PlaceCharOrder
    21:47:34 [MktDump]     stationID=60014137
    21:47:34 [MktDump]     typeID=20327
    21:47:34 [MktDump]     price=100.0000000000000
    21:47:34 [MktDump]     quantity=1
    21:47:34 [MktDump]     bid=1
    21:47:34 [MktDump]     orderRange=4294967295
    21:47:34 [MktDump]     itemID=0
    21:47:34 [MktDump]     minVolume=1
    21:47:34 [MktDump]     duration=0
    21:47:34 [MktDump]     useCorp=0
    21:47:34 [MktDump]     located:
    21:47:34 [MktDump]               None
    */

    // not sent from call, but used in transactions
    uint16 accountKey(Account::KeyType::Cash);

    _log(MARKET__DUMP, "Mkt::PlaceCharOrder()");
    call.Dump(MARKET__DUMP);

    // skill-based order count check
    if (duration->value() > 0) {
        CharacterRef ch = call.client->GetChar();
        int8 tradeLevel   = ch->GetSkillLevel(EvESkill::Trade);
        int8 retailLevel  = ch->GetSkillLevel(EvESkill::Retail);
        int8 wholeLevel   = ch->GetSkillLevel(EvESkill::Wholesale);
        int8 tycoonLevel  = ch->GetSkillLevel(EvESkill::Tycoon);
        uint32 maxOrders  = 5 + tradeLevel*4 + retailLevel*8 + wholeLevel*16 + tycoonLevel*32;

        DBQueryResult cntRes;
        uint32 ownerID = useCorp->value() ? call.client->GetCorporationID() : call.client->GetCharacterID();
        sDatabase.RunQuery(cntRes,
            "SELECT COUNT(*) FROM mktOrders WHERE ownerID = %u AND duration > 0",
            ownerID);
        DBResultRow cntRow;
        if (cntRes.GetRow(cntRow) and (uint32)cntRow.GetInt(0) >= maxOrders) {
            call.client->SendErrorMsg("You have reached the maximum number of market orders (%u). Train Trade/Retail/Wholesale/Tycoon skills to increase this limit.", maxOrders);
            return nullptr;
        }
    }

    if (useCorp->value()) {
        // Verify corp has an office in the target station
        if (!IsPlayerCorp(call.client->GetCorporationID())) {
            call.client->SendErrorMsg("You cannot trade on behalf of an NPC corporation.");
            return nullptr;
        }

        // Check corp has an office at this station
        bool hasOffice = false;
        {
            DBQueryResult officeRes;
            sDatabase.RunQuery(officeRes,
                "SELECT itemID FROM staOffices WHERE corporationID = %u AND stationID = %u",
                call.client->GetCorporationID(), stationID->value());
            hasOffice = officeRes.GetRowCount() > 0;
        }
        if (!hasOffice) {
            call.client->SendErrorMsg("Your corporation does not have an office at this station.");
            return nullptr;
        }

        accountKey = call.client->GetCorpAccountKey();
    }

    if (bid->value() and (itemID.has_value () == false || itemID.value()->value() == 0)) {  // buy
        float fStanding(0), cStanding(0);

        // is this standing order or immediate?
        if (duration->value() == 0) {
            // immediate. look for open sell order that matches all reqs (price, qty, distance, etc)
            // check distance, set order range and make station list.
            uint32 orderID(MarketDB::FindSellOrder(
                typeID->value(),
                stationID->value(),
                quantity->value(),
                price->value()
            ));

            if (orderID) {
                // found one.
                _log(MARKET__TRACE,
                    "PlaceCharOrder - Found sell order #%u in %s for %s. (type %i, price %.2f, qty %i, range %i)",
                    orderID,
                    stDataMgr.GetStationName(stationID->value()).c_str(),
                    call.client->GetName(),
                    typeID->value(),
                    price->value(),
                    quantity->value(),
                    orderRange->value()
                );

                sMktMgr.ExecuteSellOrder(
                    call.client,
                    orderID,
                    quantity->value(),
                    price->value(),
                    stationID->value(),
                    typeID->value(),
                    useCorp->value()
                );

                return nullptr;
            }

            _log(MARKET__TRACE,
                "PlaceCharOrder - Failed to satisfy buy order for %i of type %i at %.2f ISK.",
                quantity->value(),
                typeID->value(),
                price->value()
            );

            call.client->SendErrorMsg("No sell order found.");  // find/implement type name here

            return nullptr;
        }

        // range validation: Procurement skill limits buy order range
        // Only jump-based ranges are limited; special client ranges (station -1,
        // system 0, constellation 4, region 32767) pass regardless of skill.
        int8 procurementLevel = call.client->GetChar()->GetSkillLevel(EvESkill::Procurement);
        // jumpsPerSkillLevel: [5, 10, 20, 40, 60] -> skill 0 = 5 jump, skill 5 = 60
        static const uint8 buyRangeBySkill[] = { 5, 5, 10, 20, 40, 60 };
        uint8 maxBuyRange = (procurementLevel < 6) ? buyRangeBySkill[procurementLevel] : 60;
        int32 ordRange = orderRange->value();
        bool isSpecialRange = (ordRange < 0) || (ordRange == 0) || (ordRange == 4) || (ordRange == 32767);
        _log(MARKET__DUMP, "PlaceCharOrder(buy): orderRange=%d maxBuyRange=%u procurementLevel=%d isSpecial=%d",
             ordRange, maxBuyRange, procurementLevel, isSpecialRange);
        if (!isSpecialRange and (uint32)ordRange > maxBuyRange) {
            call.client->SendErrorMsg("Your Procurement skill only allows buy orders within %u jumps.", maxBuyRange);
            return nullptr;
        }

        // determine escrow amount (reduced by Margin Trading skill)
        int8 marginLevel = call.client->GetChar()->GetSkillLevel(EvESkill::MarginTrading);
        float escrowFraction = powf(0.75f, marginLevel);
        float money = (price->value() * quantity->value()) * escrowFraction;

        // set save data
        Market::SaveData data = Market::SaveData();
        data.accountKey         = accountKey;
        data.minVolume          = 1; // verify this
        data.issued             = GetFileTimeNow();
        // fill items from sent data
        data.bid                = bid->value();
        data.isCorp             = useCorp->value();
        data.typeID             = typeID->value();
        data.orderRange         = orderRange->value();
        data.ownerID            = useCorp->value() ?call.client->GetCorporationID():call.client->GetCharacterID();
        data.solarSystemID      = sDataMgr.GetStationSystem(stationID->value());
        data.regionID           = sDataMgr.GetStationRegion(stationID->value());
        data.stationID          = stationID->value();
        data.price              = price->value();
        data.volEntered         = quantity->value();
        data.volRemaining       = quantity->value();
        data.duration           = duration->value();
        data.memberID           = useCorp->value() ?call.client->GetCharacterID():0;
        data.escrow             = money;

        // these need a bit more data
        data.contraband = false;   // does this need to check region/system?  yes
        data.jumps = 1;     // not sure if this is used....

        // create buy order
        uint32 orderID(MarketDB::StoreOrder(data));
        if (orderID == 0) {
            _log(MARKET__ERROR, "PlaceCharOrder - Failed to record buy order in the DB.");
            call.client->SendErrorMsg("Failed to record the order.");
            return nullptr;
        }

        std::string reason = "DESC:  Setting up buy order in ";
        reason += stDataMgr.GetStationName(stationID->value()).c_str();
        // get data for computing broker fees
        uint8 lvl(call.client->GetChar()->GetSkillLevel(EvESkill::BrokerRelations));
        //call.client->GetChar()->GetStandingModified();
        uint32 stationOwnerID = stDataMgr.GetOwnerID (call.client->GetStationID ());
        float ownerStanding = StandingDB::GetStanding (stationOwnerID, call.client->GetCharacterID ());
        float factionStanding = 0.0f;

        if (IsNPCCorp (stationOwnerID))
            factionStanding = StandingDB::GetStanding(sDataMgr.GetCorpFaction (stationOwnerID), call.client->GetCharacterID());

        float fee = EvEMath::Market::BrokerFee(lvl, factionStanding, ownerStanding, money);
        _log(MARKET__DEBUG, "PlaceCharOrder(buy) - %s: Escrow: %.2f, Fee: %.2f", useCorp->value() ?"Corp":"Player", money, fee);

        // take monies and record actions
        if (useCorp->value()) {
            AccountService::TransferFunds(call.client->GetCorporationID(), stDataMgr.GetOwnerID(stationID->value()), fee, reason.c_str(), Journal::EntryType::Brokerfee, orderID, accountKey, Account::KeyType::Cash, call.client);
            AccountService::TransferFunds(call.client->GetCorporationID(), stDataMgr.GetOwnerID(stationID->value()), money, reason.c_str(), Journal::EntryType::MarketEscrow, orderID, accountKey, Account::KeyType::Escrow, call.client);
        } else {
            AccountService::TransferFunds(call.client->GetCharacterID(), stDataMgr.GetOwnerID(stationID->value()), fee, reason.c_str(), Journal::EntryType::Brokerfee, orderID, accountKey);
            AccountService::TransferFunds(call.client->GetCharacterID(), stDataMgr.GetOwnerID(stationID->value()), money, reason.c_str(), Journal::EntryType::MarketEscrow, orderID, accountKey, Account::KeyType::Escrow);
        }

        //send notification of new order...
        sMktMgr.InvalidateOrdersCache(call.client->GetRegionID(), typeID->value(), stationID->value());
        sMktMgr.SendOnOwnOrderChanged(call.client, orderID, Market::Action::Add, useCorp->value());
    } else {
        _log(MARKET__DUMP, "Mkt::PlaceCharOrder(): appears to be a sell order");
        // sell order
        /*if (!args.located->IsNone()) {
            //  corp item in corp hangar
            // located = [officeFolderID, officeID] or None
            PyTuple* located = args.located->AsTuple();
        }*/

        //verify that they actually have the item in the quantity specified...
        InventoryItemRef iRef = sItemFactory.GetItemRef(itemID.value ()->value());
        if (iRef.get() == nullptr) {
            _log(ITEM__ERROR, "PlaceCharOrder - Failed to find item %i for sell order.", itemID.value ()->value());

            call.client->SendErrorMsg("Unable to find item to sell.");

            return nullptr;
        }

        if (iRef->typeID() != typeID->value()) {
            _log(MARKET__MESSAGE, "PlaceCharOrder - Denying Sell of typeID %u using typeID %i.", iRef->typeID(), typeID->value());

            call.client->SendErrorMsg("Invalid sell order item type.");

            return nullptr;
        }

        if (iRef->quantity() < quantity->value()) {
            // trying to sell more than they have
            _log(MARKET__MESSAGE,
                "PlaceCharOrder - Denying inflated qty for %s",
                call.client->GetName());
                call.client->SendErrorMsg("You cannot sell %i %s when you only have %i.  If applicable, merge stacks and try again.",
                quantity->value(),
                iRef->name(),
                iRef->quantity()
            );

            return nullptr;
        }

        // verify right to sell this thing..
        if (useCorp->value()) {
            if (!IsPlayerCorp(call.client->GetCorporationID())) {
                call.client->SendErrorMsg("You cannot sell items for an NPC corp.");
                return nullptr;
            }

            accountKey = call.client->GetCorpAccountKey();
        } else {
            if ( iRef->ownerID() != call.client->GetCharacterID()) {
                _log(MARKET__WARNING,
                    "%s(%u) Tried to sell %i %s owned by %u in %s.",
                    call.client->GetName(),
                    call.client->GetCharID(),
                    iRef->quantity(),
                    iRef->name(),
                    iRef->ownerID(),
                    stDataMgr.GetStationName(stationID->value()).c_str()
                );

                call.client->SendErrorMsg("You cannot sell items you do not own.");

                return nullptr;
            }
        }

        // verify valid location
        bool itemInStationHangar(iRef->locationID() != stationID->value());
        bool itemInShip(!(call.client->GetShip()->GetMyInventory()->ContainsItem(iRef->itemID())));
        bool shipInStation(call.client->GetStationID() == stationID->value());

        if (itemInStationHangar && itemInShip && shipInStation) {
            std::string itemLoc;
            if (sDataMgr.IsStation(iRef->locationID())) {
                itemLoc = stDataMgr.GetStationName(iRef->locationID());
            } else if (sDataMgr.IsSolarSystem(iRef->locationID())) {
                itemLoc = sDataMgr.GetSystemName(iRef->locationID());
            } else {
                itemLoc = "an Invalid Location";
            }

            _log(MARKET__ERROR,
                "%s Trying to sell %s(%u) in %s through %s while in %s",
                call.client->GetName(),
                iRef->name(),
                iRef->itemID(),
                itemLoc.c_str(),
                stDataMgr.GetStationName(stationID->value()).c_str(),
                stDataMgr.GetStationName(call.client->GetStationID()).c_str()
            );

            call.client->SendErrorMsg(
                "You cannot sell %s from %s while you are in %s.  Locations mismatch",
                iRef->name(),
                stDataMgr.GetStationName(stationID->value()).c_str(),
                stDataMgr.GetStationName(call.client->GetStationID()).c_str()
            );

            return nullptr;
        }

        //TODO: verify orderRange against their skills. client may do this...verify

        // they are allowed to sell this thing...

        // is this standing order or immediate?
        if (duration->value() == 0) {
            bool completedOrder(false);

            uint32 orderID(0), origQty(quantity->value());

            // Try once — if no match exists, looping won't help and just blocks the server
            for (int i = 0; i < 1; i++) {
                _log(MARKET__DUMP, "Mkt::PlaceCharOrder(): finding buy order: %i, %i, %i, %.2f", typeID->value(), stationID->value(), quantity->value(), price->value());

                orderID = MarketDB::FindBuyOrder(typeID->value(), stationID->value(), quantity->value(), price->value());

                if (!orderID) {
                    continue;
                }

                _log(MARKET__TRACE,
                    "PlaceCharOrder - Found buy order #%u in %s for %s.",
                    orderID,
                    stDataMgr.GetStationName(stationID->value()).c_str(),
                    call.client->GetName()
                );

                completedOrder = sMktMgr.ExecuteBuyOrder(
                    call.client,
                    orderID,
                    iRef,
                    quantity->value(),
                    useCorp->value(),
                    typeID->value(),
                    stationID->value(),
                    price->value()
                );

                if (!completedOrder) {
                    continue;
                }

                _log(MARKET__DUMP, "Mkt::PlaceCharOrder(): order resolved");

                break;
            }

            if (!completedOrder) {
                _log(MARKET__ERROR, "PlaceCharOrder - failed to find a matching market order within 1000 attempts.");

                call.client->SendErrorMsg("Failed to find a suitable market order in a reasonable amount of time.");

                return nullptr;
            }

            return nullptr;
        }

        // range validation: Marketing skill limits sell order range
        // Only jump-based ranges (1..60+) are limited. Special ranges sent by the
        // client — rangeStation(-1), rangeSolarSystem(0), rangeConstellation(4),
        // rangeRegion(32767) — are NOT jump counts and must pass.
        if (duration->value() > 0) {
            int8 marketingLevel = call.client->GetChar()->GetSkillLevel(EvESkill::Marketing);
            static const uint8 sellRangeBySkill[] = { 5, 5, 10, 20, 40, 60 };
            uint8 maxSellRange = (marketingLevel < 6) ? sellRangeBySkill[marketingLevel] : 60;
            int32 ordRange = orderRange->value();
            bool isSpecialRange = (ordRange < 0) || (ordRange == 0) || (ordRange == 4) || (ordRange == 32767);
            _log(MARKET__DUMP, "PlaceCharOrder(sell): orderRange=%d maxSellRange=%u marketingLevel=%d isSpecial=%d",
                 ordRange, maxSellRange, marketingLevel, isSpecialRange);
            if (!isSpecialRange and (uint32)ordRange > maxSellRange) {
                call.client->SendErrorMsg("Your Marketing skill only allows sell orders within %u jumps.", maxSellRange);
                return nullptr;
            }
        }

        // they will be placing a sell order:

        // set save data
        Market::SaveData data = Market::SaveData();
        data.accountKey         = accountKey;
        data.minVolume          = 1; // verify this
        data.issued             = GetFileTimeNow();
        // fill items from sent data
        data.bid                = bid->value();
        data.isCorp             = useCorp->value();
        data.typeID             = typeID->value();
        data.orderRange         = orderRange->value();
        data.ownerID            = iRef->ownerID();
        data.solarSystemID      = sDataMgr.GetStationSystem(stationID->value());
        data.regionID           = sDataMgr.GetStationRegion(stationID->value());
        data.stationID          = stationID->value();
        data.price              = price->value();
        data.volEntered         = quantity->value();
        data.volRemaining       = quantity->value();
        data.duration           = duration->value();
        data.memberID           = useCorp->value() ?call.client->GetCharacterID():0;

        // check seller's permissions on the wallet if it's for corp
        if (data.isCorp) {
            int64 corpRole = call.client->GetCorpRole ();

            // make sure the user has permissions to take money from the corporation account
            if (
                (accountKey == 1000 && (corpRole & Corp::Role::AccountCanTake1) == 0) ||
                (accountKey == 1001 && (corpRole & Corp::Role::AccountCanTake2) == 0) ||
                (accountKey == 1002 && (corpRole & Corp::Role::AccountCanTake3) == 0) ||
                (accountKey == 1003 && (corpRole & Corp::Role::AccountCanTake4) == 0) ||
                (accountKey == 1004 && (corpRole & Corp::Role::AccountCanTake5) == 0) ||
                (accountKey == 1005 && (corpRole & Corp::Role::AccountCanTake6) == 0) ||
                (accountKey == 1006 && (corpRole & Corp::Role::AccountCanTake7) == 0)
            ) {
                throw UserError("CrpAccessDenied").AddFormatValue ("reason", new PyString ("You do not have access to that wallet"));
            }
        }


        // these need a bit more data
        data.contraband = iRef->contraband();   // does this need to check region/system?
        data.jumps = 1;     // not sure if this is used....

        // calculate total for broker fees
        float total = price->value() * quantity->value();
        std::string reason = "DESC:  Setting up sell order in ";
        reason += stDataMgr.GetStationName(stationID->value()).c_str();

        // get data for computing broker fees
        uint8 lvl = call.client->GetChar()->GetSkillLevel(EvESkill::BrokerRelations);

        //call.client->GetChar()->GetStandingModified();
        uint32 stationOwnerID = stDataMgr.GetOwnerID (call.client->GetStationID ());

        float ownerStanding = StandingDB::GetStanding (stationOwnerID, call.client->GetCharacterID ());
        float factionStanding = 0.0f;

        if (IsNPCCorp (stationOwnerID)) {
            factionStanding = StandingDB::GetStanding(sDataMgr.GetCorpFaction (stationOwnerID), call.client->GetCharacterID());
        }

        float fee = EvEMath::Market::BrokerFee(lvl, factionStanding, ownerStanding, total);
        _log(MARKET__DEBUG, "PlaceCharOrder(sell) - %s: Total: %.2f, Fee: %.2f", useCorp->value() ?"Corp":"Player", total, fee);

        // take monies and record actions (taxes are paid when item sells)
        if (useCorp->value()) {
            AccountService::TransferFunds(
                call.client->GetCorporationID(),
                stDataMgr.GetOwnerID(stationID->value()),
                fee,
                reason.c_str(),
                Journal::EntryType::Brokerfee,
                0,
                accountKey,
                Account::KeyType::Cash,
                call.client
            );
        } else {
            AccountService::TransferFunds(
                call.client->GetCharacterID(),
                stDataMgr.GetOwnerID(stationID->value()),
                fee,
                reason.c_str(),
                Journal::EntryType::Brokerfee,
                0,
                accountKey
            );
        }

        // store the order in the DB.
        uint32 orderID(MarketDB::StoreOrder(data));
        if (orderID == 0) {
            _log(MARKET__ERROR, "PlaceCharOrder - Failed to record sell order in the DB.");
            call.client->SendErrorMsg("Failed to record the order in the DB!");
            return nullptr;
        }

        if (iRef->quantity() == quantity->value()) {
            // take item from seller
            call.client->SystemMgr()->RemoveItemFromInventory(iRef);
            iRef->Delete();
        } else {
            //update the item.
            if (!iRef->AlterQuantity(-quantity->value(), true)) {
                _log(MARKET__ERROR, "PlaceCharOrder - Failed to consume %i units from %s", quantity->value(), iRef->name());
                return nullptr;
            }
        }

        //notify client about new order.
        sMktMgr.InvalidateOrdersCache(call.client->GetRegionID(), typeID->value(), stationID->value());
        sMktMgr.SendOnOwnOrderChanged(call.client, orderID, Market::Action::Add, useCorp->value());
    }

    return nullptr;
}

PyResult MarketProxyService::PlaceCharOrder(PyCallArgs &call, PyInt* stationID, PyInt* typeID, PyFloat* price, PyInt* quantity, PyInt* bid, PyInt* orderRange, std::optional <PyInt*> itemID, PyInt* minVolume, PyInt* duration, PyInt* useCorp, std::optional<PyRep*> located) {
    // Used when character has a corp wallet, client sends a PyInt instead of a PyBool
    return PlaceCharOrder(call, stationID, typeID, price, quantity, bid, orderRange, itemID, minVolume, duration, new PyBool(useCorp->value()), located);
}

PyResult MarketProxyService::ModifyCharOrder(PyCallArgs &call, PyInt* orderID, PyFloat* newPrice, PyInt* bid, PyInt* stationID, PyInt* solarSystemID, PyFloat* price, PyInt* range, PyInt* volRemaining, PyLong* issueDate) {
    // client coded to throw error if price > 9223372036854.0
    // we need to pull data from db for typeID and isCorp...
    Market::OrderInfo oInfo = Market::OrderInfo();
    if (!MarketDB::GetOrderInfo(orderID->value(), oInfo)) {
        _log(MARKET__ERROR, "ModifyCharOrder - Failed to get info about order #%i.", orderID->value());
        return nullptr;
    }

    // Daytrading skill: limit remote modification range
    if (call.client->GetStationID() != stationID->value()) {
        int8 daytradingLevel = call.client->GetChar()->GetSkillLevel(EvESkill::Daytrading);
        static const uint8 modRangeBySkill[] = { 0, 5, 10, 20, 40, 60 };
        uint8 maxModRange = (daytradingLevel < 6) ? modRangeBySkill[daytradingLevel] : 60;
        if (range->value() >= 0 and (uint32)range->value() > maxModRange) {
            call.client->SendErrorMsg("Your Daytrading skill does not allow modifying orders from this range.");
            return nullptr;
        }
    }

    // there is no refund in broker fees.

    // adjust balance for price change
    // If newPrice > price, charge difference; if newPrice < price, refund difference.
    double diff = newPrice->value() - price->value();
    if (diff > 0.0) {
        // raising price — charge additional escrow
        float addEscrow = static_cast<float>(diff * volRemaining->value());
        std::string reason = "DESC:  Raising price on Market Order #";
        reason += std::to_string(orderID->value());

        uint32 fromID = oInfo.isCorp ? call.client->GetCorporationID() : call.client->GetCharID();
        uint16 fromKey = oInfo.isCorp ? oInfo.accountKey : Account::KeyType::Cash;
        AccountService::TransferFunds(
            fromID,
            stDataMgr.GetOwnerID(stationID->value()),
            addEscrow,
            reason.c_str(),
            Journal::EntryType::MarketEscrow,
            orderID->value(),
            fromKey,
            Account::KeyType::Escrow
        );
    } else if (diff < 0.0) {
        // lowering price — refund difference from escrow
        float refundEscrow = static_cast<float>(-diff * volRemaining->value());
        std::string reason = "DESC:  Lowering price on Market Order #";
        reason += std::to_string(orderID->value());

        AccountService::TransferFunds(
            stDataMgr.GetOwnerID(stationID->value()),
            oInfo.isCorp ? call.client->GetCorporationID() : call.client->GetCharID(),
            refundEscrow,
            reason.c_str(),
            Journal::EntryType::MarketEscrow,
            orderID->value(),
            Account::KeyType::Escrow,
            oInfo.isCorp ? oInfo.accountKey : Account::KeyType::Cash
        );
    }

    if (!MarketDB::AlterOrderPrice(orderID->value(), newPrice->value())) {
        _log(MARKET__ERROR, "ModifyCharOrder - Failed to modify price for order #%i.", orderID->value());
        return nullptr;
    }

    sMktMgr.InvalidateOrdersCache(call.client->GetRegionID(), oInfo.typeID, oInfo.stationID);
    sMktMgr.SendOnOwnOrderChanged(call.client, orderID->value(), Market::Action::Modify, oInfo.isCorp);

    return nullptr;
}

PyResult MarketProxyService::CancelCharOrder(PyCallArgs &call, PyInt* orderID, PyInt* regionID) {
    Market::OrderInfo oInfo = Market::OrderInfo();
    if (!MarketDB::GetOrderInfo(orderID->value(), oInfo)) {
        _log(MARKET__ERROR, "CancelCharOrder - Failed to get info about order #%i.", orderID->value());
        return nullptr;
    }

    if (oInfo.isBuy) {
        float money = oInfo.price * oInfo.quantity;
        std::string reason = "DESC:  Canceling Market Order #";
        reason += std::to_string(orderID->value());

        if (oInfo.isCorp) {
            AccountService::TransferFunds(
                stDataMgr.GetOwnerID(oInfo.stationID),
                call.client->GetCorporationID(),
                money,
                reason.c_str(),
                Journal::EntryType::MarketEscrow,
                orderID->value(),
                Account::KeyType::Escrow,
                oInfo.accountKey,
                call.client
            );
        } else {
            AccountService::TransferFunds(
                stDataMgr.GetOwnerID(oInfo.stationID),
                call.client->GetCharID(),
                money,
                reason.c_str(),
                Journal::EntryType::MarketEscrow,
                orderID->value(),
                Account::KeyType::Escrow,
                Account::KeyType::Cash
            );
        }
    } else {
        if (oInfo.isCorp) {
            ItemData idata(oInfo.typeID, call.client->GetCorporationID(), locTemp, flagCorpMarket, oInfo.quantity);
            InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
            if (iRef.get() != nullptr)
                iRef->Donate(call.client->GetCorporationID(), oInfo.stationID, flagCorpMarket, true);
        } else {
            ItemData idata(oInfo.typeID, ownerStation, locTemp, flagHangar, oInfo.quantity);
            InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
            if (iRef.get() != nullptr)
                iRef->Donate(call.client->GetCharacterID(), oInfo.stationID, flagHangar, true);
        }
    }

    PyRep* order(MarketDB::GetOrderRow(orderID->value()));
    if (!MarketDB::DeleteOrder(orderID->value())) {
        _log(MARKET__ERROR, "CancelCharOrder - Failed to delete order #%i.", orderID->value());
        return nullptr;
    }

    sMktMgr.InvalidateOrdersCache(call.client->GetRegionID(), oInfo.typeID, oInfo.stationID);
    sMktMgr.SendOnOwnOrderChanged(call.client, orderID->value(), Market::Action::Expiry, oInfo.isCorp, order);

    return nullptr;
}
