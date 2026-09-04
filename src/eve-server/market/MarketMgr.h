
 /**
  * @name MarketMgr.h
  *   singleton object for storing, manipulating and managing in-game market data
  *
  * @Author:         Allan
  * @date:          19Dec17
  *
  */


#ifndef _EVE_SERVER_MARKET_MANAGER_H__
#define _EVE_SERVER_MARKET_MANAGER_H__


#include "../eve-server.h"

#include "EntityList.h"
#include "market/MarketDB.h"
#include "cache/ObjCacheService.h"

class Client;

class MarketMgr
: public Singleton< MarketMgr >
{
public:
    MarketMgr();
    ~MarketMgr();

    int Initialize(EVEServiceManager& svc);

    void Close();
    void GetInfo();
    void Process();

    void SystemStartup(SolarSystemData &data);
    void SystemShutdown(SolarSystemData &data);

    void UpdatePriceHistory();

    // fulfill market order placed by buyer to buy items (usually at reduced
    // prices).
    //
    // Updates qty in args based on order request.
    //
    // Returns true if the order is complete; false otherwise.
    bool ExecuteBuyOrder(Client* seller, uint32 orderID, InventoryItemRef iRef, uint32 quantity, bool useCorp, uint32 typeID, uint32 stationID, double price, uint16 accountKey = Account::KeyType::Cash);
    // market order placed by seller to sell items (usually at higher prices)
    void ExecuteSellOrder(Client *buyer, uint32 orderID, uint32 quantity, float price, uint32 stationID, uint32 typeID, bool useCorp);

    // --- client-less autonomous fills (trader bots / offline characters) ---
    // A docked trader bot acts as a market-maker / arbitrageur: it fills a
    // resting SELL order as the buyer (buying low) AND a resting BUY order as
    // the seller (selling high) for the same type at the same station, in one
    // atomic deal. The spread lands in the bot's wallet, both orders shrink
    // (or close), and two real mktTransactions are recorded (so the market
    // actually moves: volume + price discovery, not just book decoration).
    //
    // No Client session is required — the bot is an offline character, so all
    // ISK legs use the offline wallet path (AccountDB) and skills read straight
    // from the DB (CharacterDB::GetSkillLevel). Returns true if the deal ran.
    // botCharID : the trader bot's character (acts as middleman, never its own
    //             order — caller guarantees both orders belong to other owners).
    // askOrderID: the resting sell order to fill (bot is buyer at its price).
    // bidOrderID: the resting buy order to fill (bot is seller at its price).
    // qty       : units to move (<= volRemaining of both orders).
    // Returns the bot's net ISK profit on the deal (>=0). Returns 0 if nothing
    // was filled (order gone, wrong side, self-trade, empty book) — the caller
    // can then try the next candidate pair.
    double BotArbitrageFill(uint32 botCharID, uint32 stationID, uint32 typeID,
                            uint32 askOrderID, uint32 bidOrderID, uint32 qty);

    //forces a refresh of market data.
    void SendOnOwnOrderChanged(Client* pClient, uint32 orderID, uint8 action, bool isCorp = false, PyRep* order = nullptr);

    // stationID is optional; when non-zero the asks caches for that
    // station/system/region are also invalidated.
    void InvalidateOrdersCache(uint32 regionID, uint32 typeID, uint32 stationID = 0);

    bool NeedsUpdate()                                  { return m_timeStamp > GetFileTimeNow()?false:true; }

    PyRep* GetMarketGroups()                            { PyIncRef(m_marketGroups); return m_marketGroups; }
    // cached — invalidated by InvalidateOrdersCache when stationID is provided
    PyRep* GetStationAsks(uint32 stationID);
    PyRep* GetSystemAsks(uint32 solarSystemID);
    PyRep* GetRegionBest(uint32 regionID);
    // cached
    PyRep* GetNewPriceHistory(uint32 regionID, uint32 typeID);
    // cached
    PyRep* GetOldPriceHistory(uint32 regionID, uint32 typeID);


    // base price update method
    void SetBasePrice();         // this uses current mineral values to estimate base price of item
    void UpdateMineralPrice();
    void GetCruPrices();


protected:
    void Populate();

private:
    MarketDB m_db;
    ObjCacheService* m_cache;

    PyRep* m_marketGroups;  // static market group data

    int64 m_timeStamp;

    // markets are regional.  there are 66 regions.
    // market orders are stored as {regionID/typeID}
    //  load market data by region, sorted by system/station.
    //  station data will also store StationRef to owning/containing station for easier access later
    //  will be able to implement 'jumps' and other market conditionals
    //  ** this could take ~16m for average market data per region
};

//Singleton
#define sMktMgr \
( MarketMgr::get() )


#endif  // _EVE_SERVER_MARKET_MANAGER_H__

/*
 *    def GetStationDistance(self, stationID, getFastestRoute = True):
 *        if session.stationid == stationID:
 *            return -1
 *        station = sm.GetService('ui').GetStation(stationID)
 *        solarSystemID = station.solarSystemID
 *        regionID = sm.GetService('map').GetRegionForSolarSystem(solarSystemID)
 *        if regionID != session.regionid:
 *            return const.rangeRegion
 *        if getFastestRoute:
 *            jumps = sm.StartService('pathfinder').GetShortestJumpCountFromCurrent(solarSystemID)
 *        else:
 *            jumps = sm.StartService('pathfinder').GetJumpCountFromCurrent(solarSystemID)
 *        if jumps >= 1:
 *            return jumps
 *        return 0
 */

/*
 *    def GetSkillLimits(self):
 *        limits = {}
 *        currentOpen = 0
 *        myskills = sm.GetService('skills').MySkillLevelsByID()
 *        retailLevel = myskills.get(const.typeRetail, 0)
 *        tradeLevel = myskills.get(const.typeTrade, 0)
 *        wholeSaleLevel = myskills.get(const.typeWholesale, 0)
 *        accountingLevel = myskills.get(const.typeAccounting, 0)
 *        brokerLevel = myskills.get(const.typeBrokerRelations, 0)
 *        tycoonLevel = myskills.get(const.typeTycoon, 0)
 *        marginTradingLevel = myskills.get(const.typeMarginTrading, 0)
 *        marketingLevel = myskills.get(const.typeMarketing, 0)
 *        procurementLevel = myskills.get(const.typeProcurement, 0)
 *        visibilityLevel = myskills.get(const.typeVisibility, 0)
 *        daytradingLevel = myskills.get(const.typeDaytrading, 0)
 *        maxOrderCount = 5 + tradeLevel * 4 + retailLevel * 8 + wholeSaleLevel * 16 + tycoonLevel * 32
 *        limits['cnt'] = maxOrderCount
 *        commissionPercentage = const.marketCommissionPercentage / 100.0
 *        commissionPercentage *= 1 - brokerLevel * 0.05
 *        transactionTax = const.mktTransactionTax / 100.0
 *        transactionTax *= 1 - accountingLevel * 0.1
 *        limits['fee'] = commissionPercentage
 *        limits['acc'] = transactionTax
 *        limits['ask'] = jumpsPerSkillLevel[marketingLevel]
 *        limits['bid'] = jumpsPerSkillLevel[procurementLevel]
 *        limits['vis'] = jumpsPerSkillLevel[visibilityLevel]
 *        limits['mod'] = jumpsPerSkillLevel[daytradingLevel]
 *        limits['esc'] = 0.75 ** marginTradingLevel
 *        return limits
 */
