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

#include "EVE_Corp.h"
#include "EVE_Mail.h"
#include "marshal/EVEMarshal.h"

#include "Client.h"
#include "ConsoleCommands.h"
#include "EntityList.h"
#include "EVE_Wallet.h"
#include "account/AccountService.h"
#include "inventory/ItemFactory.h"
#include "EVEServerConfig.h"
#include "ServiceDB.h"
#include "agents/Agent.h"
#include "exploration/Probes.h"
#include "map/MapDB.h"
#include "market/MarketMgr.h"
#include "market/MarketBotMgr.h"
#include "npc/BotMgr.h"
#include "faction/WarRegistryService.h"
#include "missions/MissionDataMgr.h"
#include "incursion/IncursionMgr.h"
#include "expedition/ExpeditionMgr.h"
#include "standing/StandingMgr.h"
#include "station/Station.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/cosmicMgrs/CivilianMgr.h"
#include "system/cosmicMgrs/WormholeMgr.h"
#include "system/cosmicMgrs/ManagerDB.h"
#include "corporation/CorporationDB.h"
#include "alliance/AllianceDB.h"

EntityList::EntityList()
: m_services(nullptr),
m_targTimer(0, true),
m_stampTimer(0, true),
m_minuteTimer(0, true),
m_startTime(0),
m_npcs(0),
m_stamp(1000),   /* arbitrary.  start at 1k.  in seconds.  used for destiny and client counters */
m_minutes(0),
m_connections(0),
m_clientSeedID(0)
{
    m_agents.clear();
    m_probes.clear();
    m_clients.clear();
    m_players.clear();
    m_systems.clear();
    m_stations.clear();
    m_targMgrs.clear();
    m_corpMembers.clear();

    m_shipTracking = sConfig.debug.UseShipTracking;
}

EntityList::~EntityList() {
    sLog.Green("   ServerShutdown", " Complete.");
}

void EntityList::Initialize() {
    m_startTime = GetFileTimeNow();

    /* start the timers */
    m_targTimer.Start(250);     // testing targeting and scan probes at 4/sec
    m_stampTimer.Start(1000);   // 1hz tic timer
    m_minuteTimer.Start(60000); // does this need to be accurate?

    m_clientSeedID = ServiceDB::SetClientSeed();
    sLog.Green( "       ServerInit", "ClientSeed Initialized." );

    if (is_log_enabled(SERVER__STACKTRACE))
        sConfig.debug.StackTrace = true;

    // seed crpRoles from existing characters
    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT IGNORE INTO crpRoles (characterID, roleID)"
        " SELECT characterID, corpRole FROM chrCharacters WHERE corpRole > 0");

    sLog.Blue("       EntityList", "Entity Manager Initialized.");
}

void EntityList::Shutdown() {
    /** @todo finish this....
     * halt server called from admin client. (gm command ingame)
     * call d'tor on all connected clients
     * server run loop will exit after control is returned from this function, which will clean up remaining items.
     */
    for (auto cur : m_clients)
        SafeDelete(cur);

    m_clients.clear();
}

void EntityList::Close()
{
    if (m_clients.size() > 0) {
        sLog.Yellow("       EntityList", "Cleaning up %lu clients, %lu systems, %lu agents, and %lu stations", \
                    m_clients.size(), m_systems.size(), m_agents.size(), m_stations.size());
    } else {
        sLog.Green("       EntityList", "Cleaning up %lu clients, %lu systems, %lu agents, and %lu stations", \
                    m_clients.size(), m_systems.size(), m_agents.size(), m_stations.size());
    }

    for (auto cur : m_clients)
        SafeDelete(cur);

    for (auto cur : m_agents)
        SafeDelete(cur.second);

    for (auto cur : m_systems) {
        cur.second->UnloadSystem();
        SafeDelete(cur.second);
    }

    sLog.Warning("       EntityList", "Entity List has been closed." );
}

/* m_clients is used to search for online players and numerous other things.
 *  the problem here is any searching is done thru iteration, which can get expensive.
 *  however, clients are added before their char is selected, so there is no charID for map placement.
 *    maybe use m_clients for basic Process() calls and use m_players for character/client searching
 *
 * update:  done and working very well.
 */

void EntityList::Add( Client* pClient ) {
    ++m_connections;
    if (pClient != nullptr)
        m_clients.push_back(pClient);
}

void EntityList::Remove(Client* pClient) {
    /* note:  will get expensive for many clients  */
    std::vector<Client*>::iterator itr = m_clients.begin();
    for (; itr != m_clients.end(); ++itr)
        if ((*itr) == pClient) {
            m_clients.erase(itr);
            return;
        }
}

void EntityList::AddPlayer(Client* pClient)
{
    if (pClient != nullptr)
        if (pClient->IsValidSession()) {
            m_players.emplace(pClient->GetCharacterID(), pClient);
            if (IsPlayerCorp(pClient->GetCorporationID())) {
                corpRole role;
                role.emplace(pClient, pClient->GetCorpRole());
                m_corpMembers.emplace(pClient->GetCorporationID(), role);
            }
        } else {
            m_players.emplace(pClient->GetCharID(), pClient);
            // make note about invalid session and failure to add player to corp roles.
            //   this is nbd if player is in npc corp or in player corp with no roles
        }
}

void EntityList::RemovePlayer(Client* pClient)
{
    if (pClient != nullptr)
        if (pClient->IsValidSession()) {
            m_players.erase(pClient->GetCharacterID());
            // remove player from corp map, if applicable
            std::map<uint32, corpRole>::iterator itr = m_corpMembers.find(pClient->GetCorporationID());
            if (itr != m_corpMembers.end())
                itr->second.erase(pClient);
        } else {
            m_players.erase(pClient->GetCharID());
        }
}


void ProcessInsuranceExpiry()
{
    // Find expired insurance policies
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT shipID, shipName, ownerID FROM shipInsurance WHERE endDate < %.0f",
        (double)GetFileTimeNow()))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 shipID  = row.GetUInt(0);
        std::string shipName = row.GetText(1);
        uint32 ownerID = row.GetUInt(2);

        // Send InsuranceExpiration notification
        PyDict* data = new PyDict();
        data->SetItemString("shipID", new PyInt(shipID));
        data->SetItemString("shipName", new PyString(shipName));
        sEntityList.CreateNotification(ownerID, Notify::Types::InsuranceExpiration, corpSCC, data);

        _log(CORP__MESSAGE, "InsuranceExpiry: policy for %s (ship %u) expired — owner %u notified", shipName.c_str(), shipID, ownerID);

        // Delete expired policy
        DBerror err;
        sDatabase.RunQuery(err, "DELETE FROM shipInsurance WHERE shipID = %u", shipID);
    }
}

void ProcessAutoPay()
{
    // Retry cooldown: track last BillOutOfMoney notification per billID
    static std::map<uint32, double> s_billNotifyTimes;

    // Auto-pay bills for corporations with auto-pay enabled
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT billID, debtorID, creditorID, billTypeID, amount, externalID, externalID2 "
        "FROM billsPayable WHERE paid = 0 AND dueDateTime < %.0f",
        (double)GetFileTimeNow()))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 billID     = row.GetUInt(0);
        uint32 debtorID   = row.GetUInt(1);
        uint32 creditorID = row.GetUInt(2);
        uint32 billType   = row.GetUInt(3);
        double amount     = row.GetDouble(4);
        uint32 extID      = row.GetUInt(5);
        uint32 extID2     = row.GetUInt(6);

        // Skip war bills — handled by ProcessWarBills()
        if (billType == Corp::BillType::WarBill)
            continue;

        // Only corp debtors have auto-pay settings
        DBQueryResult autoRes;
        if (!sDatabase.RunQuery(autoRes,
            "SELECT market, rental, broker, war, alliance, sov "
            "FROM crpAutoPay WHERE corporationID = %u", debtorID))
            continue;

        DBResultRow autoRow;
        if (!autoRes.GetRow(autoRow))
            continue;

        bool autoPay = false;
        switch (billType) {
            case Corp::BillType::MarketFine:              autoPay = autoRow.GetBool(0); break;
            case Corp::BillType::RentalBill:              autoPay = autoRow.GetBool(1); break;
            case Corp::BillType::BrokerBill:              autoPay = autoRow.GetBool(2); break;
            case Corp::BillType::AllianceMaintainanceBill: autoPay = autoRow.GetBool(4); break;
            case Corp::BillType::SovereigntyMarker:        autoPay = autoRow.GetBool(5); break;
        }
        if (!autoPay)
            continue;

        // Check corp wallet balance
        uint16 accountKey = Account::KeyType::Cash;
        double balance = AccountDB::GetCorpBalance(debtorID, accountKey);
        if (balance < amount) {
            // Retry cooldown — notify at most once per hour per bill
            double now = GetFileTimeNow();
            auto it = s_billNotifyTimes.find(billID);
            if (it != s_billNotifyTimes.end() and (now - it->second) < EvE::Time::Hour) {
                _log(CORP__MESSAGE, "AutoPay: corp %u still short for bill %u (%.2f ISK, balance %.2f) — skipped notify",
                    debtorID, billID, amount, balance);
                continue;
            }
            s_billNotifyTimes[billID] = now;

            _log(CORP__MESSAGE, "AutoPay: corp %u insufficient funds for bill %u (%.2f ISK)", debtorID, billID, amount);

            // Notify debtor corp about insufficient funds
            PyDict* data = new PyDict();
            data->SetItemString("billID", new PyInt(billID));
            data->SetItemString("debtorID", new PyInt(debtorID));
            data->SetItemString("creditorID", new PyInt(creditorID));
            data->SetItemString("billTypeID", new PyInt(billType));
            data->SetItemString("amount", new PyFloat(amount));
            data->SetItemString("externalID", new PyInt(extID));
            data->SetItemString("externalID2", new PyInt(extID2));
            sEntityList.CreateNotification(debtorID, Notify::Types::BillOutOfMoney, 0, data);
            continue;
        }

        // Transfer funds
        AccountService::TransferFunds(
            debtorID, creditorID, amount,
            "Auto-payment of bill", billType, billID,
            accountKey, Account::KeyType::Cash, nullptr);

        // Mark bill as paid
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE billsPayable SET paid = 1 WHERE billID = %u", billID);

        _log(CORP__MESSAGE, "AutoPay: corp %u auto-paid bill %u (type %u) for %.2f ISK", debtorID, billID, billType, amount);

        // Clear cooldown entry on successful payment (if any)
        s_billNotifyTimes.erase(billID);

        // Notify creditor
        PyDict* data = new PyDict();
        data->SetItemString("billID", new PyInt(billID));
        data->SetItemString("debtorID", new PyInt(debtorID));
        data->SetItemString("creditorID", new PyInt(creditorID));
        data->SetItemString("billTypeID", new PyInt(billType));
        data->SetItemString("amount", new PyFloat(amount));
        data->SetItemString("externalID", new PyInt(extID));
        data->SetItemString("externalID2", new PyInt(extID2));
        sEntityList.CreateNotification(creditorID, Notify::Types::BillPaidCorpAll, debtorID, data);
    }
}

void CheckWarDecay()
{
    // End wars where the war bill is overdue and unpaid
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT wr.warID FROM warRegistry wr "
        "JOIN billsPayable bp ON wr.billID = bp.billID "
        "WHERE bp.paid = 0 AND bp.dueDateTime < %.0f AND wr.timeFinished = 0",
        GetFileTimeNow()))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 warID = row.GetUInt(0);
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE warRegistry SET timeFinished = %.0f, retracted = 1 WHERE warID = %u",
            GetFileTimeNow(), warID);
        sLog.Warning("WarDecay", "War %u ended due to unpaid bill", warID);
    }
}

void CheckExpiredAuctions()
{
    // Find expired auction contracts (type=4, status=0=Outstanding, dateExpired < now)
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT contractId FROM ctrContracts"
        " WHERE contractType = 4 AND status = 0 AND dateExpired < %.0f",
        GetFileTimeNow()))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 cID = row.GetUInt(0);

        // Check if there are any bids
        DBQueryResult bidRes;
        DBResultRow bidRow;
        if (sDatabase.RunQuery(bidRes,
            "SELECT bidderID, amount FROM ctrBids WHERE contractID = %u ORDER BY amount DESC LIMIT 1", cID))
        {
            if (bidRes.GetRow(bidRow)) {
                // Has bids — finish auction with winner
                uint32 winnerID = bidRow.GetUInt(0);
                double amount = bidRow.GetDouble(1);

                // Get issuer
                DBQueryResult issuerRes;
                DBResultRow issuerRow;
                uint32 issuerID = 0;
                if (sDatabase.RunQuery(issuerRes,
                    "SELECT issuerID FROM ctrContracts WHERE contractId = %u", cID))
                    if (issuerRes.GetRow(issuerRow)) issuerID = issuerRow.GetUInt(0);

                // Update status
                DBerror err;
                sDatabase.RunQuery(err,
                    "UPDATE ctrContracts SET status = 4, dateCompleted = %.0f, acceptorID = %u WHERE contractId = %u",
                    GetFileTimeNow(), winnerID, cID);

                // Pay issuer
                AccountService::TransferFunds(winnerID, issuerID, amount,
                    "Auction payment", Journal::EntryType::ContractAuctionSold, cID);

                // Transfer items to winner
                DBQueryResult itemRes;
                sDatabase.RunQuery(itemRes,
                    "SELECT itemID FROM ctrItems WHERE contractId = %u AND inCrate = 1", cID);
                DBResultRow itemRow;
                while (itemRes.GetRow(itemRow)) {
                    InventoryItemRef iRef = sItemFactory.GetItemRef(itemRow.GetUInt(0));
                    if (iRef.get() != nullptr) {
                        iRef->ChangeOwner(winnerID, true);
                        DBQueryResult stationRes;
                        sDatabase.RunQuery(stationRes,
                            "SELECT startStationID FROM ctrContracts WHERE contractId = %u", cID);
                        DBResultRow stationRow;
                        uint32 stationID = 0;
                        if (stationRes.GetRow(stationRow))
                            stationID = stationRow.GetUInt(0);
                        if (stationID > 0)
                            iRef->Move(stationID, flagHangar, true);
                    }
                }

                sLog.Warning("ExpiredAuction", "Auction %u finished - winner %u paid %.2f ISK", cID, winnerID, amount);
            } else {
                // No bids — reject
                DBerror err;
                sDatabase.RunQuery(err,
                    "UPDATE ctrContracts SET status = 6, dateCompleted = %.0f WHERE contractId = %u",
                    GetFileTimeNow(), cID);

                // Return items to issuer
                DBQueryResult itemRes;
                sDatabase.RunQuery(itemRes,
                    "SELECT itemID FROM ctrItems WHERE contractId = %u AND inCrate = 1", cID);
                DBResultRow itemRow;
                while (itemRes.GetRow(itemRow)) {
                    InventoryItemRef iRef = sItemFactory.GetItemRef(itemRow.GetUInt(0));
                    if (iRef.get() != nullptr) {
                        DBQueryResult stationRes;
                        sDatabase.RunQuery(stationRes,
                            "SELECT issuerID, startStationID FROM ctrContracts WHERE contractId = %u", cID);
                        DBResultRow stationRow;
                        uint32 issuerID = 0, stationID = 0;
                        if (stationRes.GetRow(stationRow)) {
                            issuerID = stationRow.GetUInt(0);
                            stationID = stationRow.GetUInt(1);
                        }
                        if (issuerID > 0) iRef->ChangeOwner(issuerID, true);
                        if (stationID > 0) iRef->Move(stationID, flagHangar, true);
                    }
                }
                sLog.Warning("ExpiredAuction", "Auction %u expired with no bids - items returned", cID);
            }
        }
    }
}

void CheckVoteExpiry()
{
    // find expired corp votes that haven't been processed yet
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        " SELECT voteCaseID, corporationID, voteType, endDateTime"
        " FROM crpVoteItems"
        " WHERE inEffect = 0 AND endDateTime < %.0f",
        GetFileTimeNow()))
        return;

    DBResultRow row;
    while (res.GetRow(row))
    {
        uint32 voteCaseID = row.GetUInt(0);
        uint32 corpID = row.GetUInt(1);
        uint32 voteType = row.GetUInt(2);

        // count votes per option, pick winner
        DBQueryResult optRes;
        sDatabase.RunQuery(optRes,
            " SELECT optionID, votesFor, parameter FROM crpVoteOptions"
            " WHERE voteCaseID = %u"
            " ORDER BY votesFor DESC LIMIT 1",
            voteCaseID);

        DBResultRow optRow;
        if (!optRes.GetRow(optRow) or optRow.GetUInt(1) == 0)
        {
            // no votes cast, mark as expired with no effect
            DBerror err;
            sDatabase.RunQuery(err,
                "UPDATE crpVoteItems SET inEffect = 0, actedUpon = 1, timeActedUpon = %.0f"
                " WHERE voteCaseID = %u",
                GetFileTimeNow(), voteCaseID);
            continue;
        }

        uint32 targetParam = optRow.GetUInt(2);

        // execute the vote result based on type
        if (voteType == Corp::VoteType::CEO)
        {
            // targetParam is the new CEO characterID
            DBerror err;
            sDatabase.RunQuery(err,
                " UPDATE crpCorporation SET ceoID = %u"
                " WHERE corporationID = %u",
                targetParam, corpID);
        }

        // mark vote as passed and acted upon
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE crpVoteItems SET inEffect = 1, actedUpon = 1, timeActedUpon = %.0f"
            " WHERE voteCaseID = %u",
            GetFileTimeNow(), voteCaseID);
    }
}


void EntityList::Process() {
    Client* pClient(nullptr);
    std::vector<Client*>::iterator citr = m_clients.begin();
    while (citr != m_clients.end()) {
        if ((*citr)->ProcessNet()) {
            ++citr;
        } else {
            pClient = *citr;
            citr = m_clients.erase(citr);
            SafeDelete(pClient);
        }
    }

    if (m_targTimer.Check()) {
        std::unordered_map<SystemEntity*, TargetManager*>::iterator titr = m_targMgrs.begin();
        while (titr != m_targMgrs.end()) {
            if (titr->second->Process()) {
                ++titr;
            } else {
                titr = m_targMgrs.erase(titr);
            }
        }
        std::map<uint32, ProbeSE*>::iterator pitr = m_probes.begin();
        while (pitr != m_probes.end()) {
            if (pitr->second->ProcessTic()) {
                ++pitr;
            } else {
                pitr = m_probes.erase(pitr);
            }
        }
    }

    /* check for 1Hz timer tic */
    if (m_stampTimer.Check()) {
        double profileStartTime(GetTimeUSeconds());

        ++m_stamp;

        for (auto cur : m_players)
            if (cur.second->IsValidSession())   // verify client is constructed before calling ProcessClient() on it
                cur.second->ProcessClient();

        // Simulated players: top up active systems with AI pilots on the 1Hz tic.
        sBotMgr.Process();

    /** @todo test for adding OpenMP here to enable MP per system. */
    // this wont work....possibility of removing systems, therefore invalidating the iterator.
    // bad things can happen if this is running parallel on MP
    //#pragma omp parallel  // starts a new team
        std::map<uint32, SystemManager*>::iterator itr = m_systems.begin();
        while (itr != m_systems.end()) {
            if (itr->second == nullptr) { /* this shouldnt happen.  log error to make note */
                sLog.Error(" EntityList::Proc", "Deleting System %u", itr->first);
                itr = m_systems.erase(itr);
                continue;
            } else if (!itr->second->ProcessTic()) {    /* Process each loaded system */
                itr->second->UnloadSystem();
                SafeDelete(itr->second);
                itr = m_systems.erase(itr);
                continue;
            }
            ++itr;
        }

        // these need 1Hz tics
        sCivMgr.Process();
        sBubbleMgr.Process();
        sStandingMgr.ProcessResearch();

        // these minute tics do not need to be precise
        if (m_minuteTimer.Check()) {
            ++m_minutes;
            sMissionDataMgr.Process();  // 1m
            sIncursionMgr.Process();    // 1m
            sExpMgr.Process();          // 1m — expedition expiry
            ProcessAutoPay();
            ProcessInsuranceExpiry();
            CheckWarDecay();
            ProcessWarBills();
            CheckExpiredAuctions();
            CheckVoteExpiry();

            // spawn FW plexes in loaded FW systems (every 5 minutes to catch newly loaded systems)
            if (m_minutes % 5 == 0) {
                static std::set<uint32> fwSystems;
                static uint32 lastRefresh = 0;
                if (fwSystems.empty() or (m_minutes - lastRefresh) > 30) {
                    fwSystems.clear();
                    DBQueryResult fwRes;
                    sDatabase.RunQuery(fwRes, "SELECT systemID FROM facWarSystems");
                    DBResultRow fwRow;
                    while (fwRes.GetRow(fwRow))
                        fwSystems.insert(fwRow.GetUInt(0));
                    lastRefresh = m_minutes;
                }
                std::map<uint32, SystemManager*>::iterator sysItr = m_systems.begin();
                while (sysItr != m_systems.end()) {
                    uint32 sysID = sysItr->first;
                    if (fwSystems.find(sysID) == fwSystems.end()) {
                        ++sysItr;
                        continue;
                    }
                    AnomalyMgr* anom = sysItr->second->GetAnomMgr();
                    if (anom == nullptr or anom->HasFWAnomalies()) {
                        ++sysItr;
                        continue;
                    }
                    uint8 plexCount = (sysItr->second->GetSystemSecurityRating() > 0.45f) ? 2 : 3;
                    for (uint8 i = 0; i < plexCount; ++i) {
                        GPoint pos(
                            MakeRandomFloat(-1e9f, 1e9f),
                            MakeRandomFloat(-1e9f, 1e9f),
                            MakeRandomFloat(-1e9f, 1e9f)
                        );
                        // Random plex type weighted toward smaller sizes
                        uint8 plexType = 0;  // Scout
                        float r = MakeRandomFloat();
                        if      (r < 0.10f) plexType = 3;  // Large 10%
                        else if (r < 0.30f) plexType = 2;  // Medium 20%
                        else if (r < 0.55f) plexType = 1;  // Small 25%
                        else                plexType = 0;  // Scout 45%
                        std::string sigID = "FW_" + std::to_string(sysID) + "_" + std::to_string(i);
                        std::string name = "FW Plex " + std::to_string(i + 1);
                        anom->AddFWAnomaly(sigID, pos, name, 0);
                        anom->SetFWAnomalyType(sigID, plexType);
                    }
                    ++sysItr;
                }
            }

            if (m_minutes % 5 == 0) { // ~5m
                sWHMgr.Process();
                // write something to tic corps vote cases.
                for (auto cur : m_systems)
                    cur.second->UpdateData();   // update active system timers and dynamic data every 5m
            }
            if (m_minutes % 15 == 0) { // ~15m
                sMktBotMgr.Process();  // 15m to 30m ---marketbot update; enabled this for timer checks in process
                sConsole.UpdateStatus();
            }
            if (m_minutes % 60 == 0) { // ~1h
                MapDB::ManipulateTimeData();
                sMktMgr.Process();  // not used - does nothing at this time
            }
        }

        if (sConfig.debug.UseProfiling)
            sProfiler.AddTime(Profile::entityS, GetTimeUSeconds() - profileStartTime);
    }
}

SystemManager* EntityList::FindOrBootSystem(uint32 systemID) {
    if (!sDataMgr.IsSolarSystem(systemID)) {
        _log(SERVER__INIT_ERR, "BootSystem() called with invalid systemID (%u)", systemID);
        return nullptr;
    }

    std::map<uint32, SystemManager*>::iterator itr = m_systems.find(systemID);
    if (itr != m_systems.end())
        return itr->second;

    SystemManager* pSM = new SystemManager(systemID, *m_services);
    if ((pSM == nullptr) or (!pSM->BootSystem())) {
        _log(SERVER__INIT_ERR, "BootSystem() - Booting system %u failed", systemID);
        SafeDelete(pSM);
        return nullptr;
    }

    _log(SERVER__INIT, "BootSystem() - Booted system %u", systemID);
    m_systems[systemID] = pSM;
    return pSM;
}

// cannot put add/remove station in header due to incomplete StationItemRef class
void EntityList::AddStation(uint32 stationID, StationItemRef itemRef) {
    m_stations[stationID] = itemRef;
}

void EntityList::RemoveStation(uint32 stationID) {
    m_stations.erase(stationID);
}

Agent* EntityList::GetAgent(uint32 agentID) {
    std::map<uint32, Agent*>::iterator res = m_agents.find(agentID);
    if (res != m_agents.end())
        return res->second;

    Agent* pAgent = new Agent(agentID);
    if (!pAgent->Load()) {
        delete pAgent;
        return nullptr;
    }
    m_agents[agentID] = pAgent;
    return pAgent;
}

void EntityList::GetClients(std::vector<Client*> &result) const {
    for (auto cur : m_players)
        result.push_back(cur.second);
}

void EntityList::GetCorpClients(std::vector<Client*> &result, uint32 corpID) const {
    std::map<uint32, corpRole>::const_iterator cItr = m_corpMembers.find(corpID);
    if (cItr == m_corpMembers.end())
        return;

    corpRole::const_iterator itr = cItr->second.begin(), end = cItr->second.end();
    while (itr != end) {
        if (itr->first != nullptr)
            result.push_back(itr->first);
        ++itr;
    }
}

// this method is corrected, as stations have their own guestlist now.
void EntityList::GetStationGuestList(uint32 stationID, std::vector<Client*> &result) const {
    std::map<uint32, StationItemRef>::const_iterator itr = m_stations.find(stationID);
    if (itr != m_stations.end())
        itr->second->GetGuestList(result);
}

bool EntityList::IsOnline(uint32 charID)
{
    if (m_players.find(charID) == m_players.end())
        return false;
    return true;
}

PyRep* EntityList::PyIsOnline(uint32 charID)
{
    if (m_players.find(charID) == m_players.end())
        return PyStatic.NewFalse();
    return PyStatic.NewTrue();
}

Client* EntityList::FindClientByCharID(uint32 charID) const
{
    std::map<uint32, Client*>::const_iterator itr = m_players.find(charID);
    if (itr != m_players.end())
        return itr->second;
    return nullptr;
}

Client* EntityList::FindClientByAccountID(uint32 accountID) const
{
    for (auto itr : m_players) {
        Client* c = itr.second;
        if (c != nullptr && c->GetUserID() == (int32)accountID)
            return c;
    }
    return nullptr;
}

StationItemRef EntityList::GetStationByID(uint32 stationID) {
    std::map<uint32, StationItemRef>::iterator res = m_stations.find(stationID);
    if (res != m_stations.end())
        return res->second;
    return StationItemRef(nullptr);
}

std::string EntityList::GetAnomalyID()
{
    // these should be totally unique.  design a way to enforce this
    std::string str1 = "", str2 = "";
    for (uint8 i = 0; i < 3; ++i) {
        str1 += alphaList[MakeRandomInt(0,25)];    //rand() % sizeof(alphaList) - 1
        str2 += std::to_string(MakeRandomInt(0,9));
    }

    std::string res = str1;
    res += "-";
    res += str2;
    // not sure if we need to keep track of these IDs...
    //m_anomIDs.push_back(res);
    return res;
}

void EntityList::GetUpTime( std::string& time )
{
    float seconds = m_stamp - 1000;
    float minutes = seconds/60;
    float hours = minutes/60;
    float days = hours/24;
    float weeks = days/7;
    float months = days/30;

    int s(fmod(seconds, 60));
    int m(fmod(minutes, 60));
    int h(fmod(hours, 24));
    int d(fmod(days, 7));
    int w(fmod(weeks, 4));
    int M(fmod(months, 12));

    std::ostringstream uptime;
    if (M) {
        uptime << M << "M" << w << "w" << d << "d" << h << "h" << m << "m" << s << "s";
    } else if (w) {
        uptime << w << "w" << d << "d" << h << "h" << m << "m" << s << "s";
    } else if (d) {
        uptime << d << "d" << h << "h" << m << "m" << s << "s";
    } else if (h) {
        uptime << h << "h" << m << "m" << s << "s";
    } else if (m) {
        uptime << m << "m" << s << "s";
    } else {
        uptime << s << "s";
    }

    //std::shared_ptr<const char*> ret = uptime.str().c_str();
    time = uptime.str();
}


// this is my answer to the crazy looping of Multicast shit...
void EntityList::CorpNotify(uint32 corpID, uint8 bCastType, const char* notifyType, const char* idType, PyTuple* payload) const
{
    // make sure this is player corp (which it really should be, but just in case....)
    if (IsNPCCorp(corpID))
        return;
    std::map<uint32, Client*> cMap;
    std::map<uint32, corpRole>::const_iterator cItr = m_corpMembers.find(corpID);
    if (cItr == m_corpMembers.end()) {
        PySafeDecRef(payload);
        return; // no corp members online now.  nothing to do here.
    }

    // determine who in corp needs to be notified
    using namespace Notify::Types;
    //using namespace Corp::Role;
    // auto doesnt work here...dunno why yet.
    corpRole::const_iterator itr = cItr->second.begin(), end = cItr->second.end();
    switch (bCastType) {
        case CorpNews:
        case CorpNewCEO:
        case CharLeftCorp: {
            // all members?
            while (itr != end) {
                cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case CorpAppNew:
        case CorpAppReject:
        case CorpAppAccept: {
            // who else wants/needs this?
            // PersonnelManager is only role that can view corp applications
            while (itr != end) {
                //if ((itr->second & Corp::Role::Director) == Corp::Role::Director)
                //    cMap.insert(std::make_pair(std::make_pair(itr->first->GetCharacterID(), itr->first)));
                if ((itr->second & Corp::Role::PersonnelManager) == Corp::Role::PersonnelManager)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case CorpVote: {
            // all corp members can vote
            while (itr != end) {
                cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;

        // unused yet.  (not coded or not understood  ...mostly the latter at this point in corp code)
        case CharMedal:
        case AllMaintenanceBill:
        case AllWarDeclared:
        case AllWarSurrender:
        case AllWarRetracted:
        case AllWarInvalidated:
        case CharBill:
        case CorpAllBill:
        case BillOutOfMoney:
        case BillPaidChar:
        case BillPaidCorpAll:
        case CorpTaxChange:
        case CorpDividend:
        case CorpVoteCEORevoked:
        case CorpWarDeclared:
        case CorpWarFightingLegal:
        case CorpWarSurrender:
        case CorpWarRetracted:
        case CorpWarInvalidated:
        case ContainerPassword:
        case SovAllClaimFail:
        case SovCorpClaimFail:
        case SovAllBillLate:
        case SovCorpBillLate:
        case SovAllClaimLost:
        case SovCorpClaimLost:
        case SovAllClaimAquired:
        case SovCorpClaimAquired:
        case AllAnchoring:
        case AllStructVulnerable:
        case AllStrucInvulnerable:
        case SovDisruptor:
        case CorpStructLost:
        case CorpOfficeExpiration:
        case FWCorpJoin:
        case FWCorpLeave:
        case FWCorpKick:
        case FWCharKick:
        case FWCorpWarning:
        case FWCharWarning:
        case FWCharRankLoss:
        case FWCharRankGain:
        case FWAllianceWarning:
        case FWAllianceKick:
        case TransactionReversal:
        case Reimbursement:
        case TowerAlert:
        case TowerResourceAlert:
        case StationAggression1:
        case StationStateChange:
        case StationConquer:
        case StationAggression2:
        case FacWarCorpJoinRequest:
        case FacWarCorpLeaveRequest:
        case FacWarCorpJoinWithdraw:
        case FacWarCorpLeaveWithdraw:
        case CorpLiquidation:
        case SovereigntyTCUDamage:
        case SovereigntySBUDamage:
        case SovereigntyIHDamage:
        case ContactAdd:
        case ContactEdit:
        case CorpKicked:
        case OrbitalAttacked:
        case OrbitalReinforced:
        case OwnershipTransferred:
            break;

        // internal corp notifications
        case FactoryJob: {      // factory job completion added to calendar
            // who else wants/needs this?
            //  lets start with factory manager, and may have to add later
            while (itr != end) {
                if ((itr->second & Corp::Role::FactoryManager) == Corp::Role::FactoryManager)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case MarketOrder: {
            // who else wants/needs this?
            //  lets start with traders, and may have to add later
            while (itr != end) {
                if ((itr->second & Corp::Role::Trader) == Corp::Role::Trader)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case WalletChange: {
            while (itr != end) {
                if ((itr->second & Corp::Role::Accountant) == Corp::Role::Accountant)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                if ((itr->second & Corp::Role::Auditor) == Corp::Role::Auditor)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                // this may need to check if player has access to division changed - will require a LOT more code
                if ((itr->second & Corp::Role::JuniorAccountant) == Corp::Role::JuniorAccountant)
                    cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case ItemUpdateStation: {
            // all members?
            while (itr != end) {
                cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
        case ItemUpdateSystem: {
            // all members?
            while (itr != end) {
                cMap.emplace(itr->first->GetCharacterID(), itr->first);
                ++itr;
            }
        } break;
    }

    for (auto cur : cMap) {
        PyIncRef(payload);
        cur.second->SendNotification( notifyType, idType, payload, false );   // are any of these sequenced?
    }

    PyDecRef(payload);
}

void EntityList::Broadcast(const char* notifyType, const char* idType, PyTuple** payload) const {
    //build a little notification out of it.
    EVENotificationStream notify;
        notify.remoteObject = 1;
        notify.args = *payload;
    payload = nullptr;    //consumed

    //now sent it to the client
    PyAddress dest;
        dest.type = PyAddress::Broadcast;
        dest.service = notifyType;
        dest.bcast_idtype = idType;
    Broadcast(dest, notify);
}

void EntityList::Broadcast(const PyAddress &dest, EVENotificationStream &noti) const {
    for (auto cur : m_players)
        cur.second->SendNotification(dest, noti);
}

void EntityList::Multicast(const character_set &cset, const PyAddress &dest, EVENotificationStream &noti) const {
    std::map<uint32, Client*>::const_iterator itr = m_players.begin();
    for (auto cur : cset) {
        itr = m_players.find(cur);
        if (itr != m_players.end())
            itr->second->SendNotification(dest, noti);
    }
}

// updated to remove looping thru entire client list for each call....still needs work
void EntityList::Multicast( const char* notifyType, const char* idType, PyTuple** in_payload, NotificationDestination target, uint32 targID, bool seq )
{
    PyTuple* payload = *in_payload;
    in_payload = nullptr;

    std::vector<Client*> cVec;
    cVec.clear();
    switch( target ) {
        case NOTIF_DEST__LOCATION: {
            if (sDataMgr.IsStation(targID)) {
                GetStationGuestList(targID, cVec);
            } else if (sDataMgr.IsSolarSystem(targID)) {
                SystemManager* pSysMgr = FindOrBootSystem(targID);
                if (pSysMgr == nullptr)
                    break;
                pSysMgr->GetClientList(cVec);
            } else {
                sLog.Error("EntityList::Multicast 1", "DEST__LOCATION - location %u is neither station nor system", targID);
                EvE::traceStack();
            }
        } break;
        case NOTIF_DEST__CORPORATION: {
            std::map<uint32, corpRole>::const_iterator cItr = m_corpMembers.find(targID);
            if (cItr == m_corpMembers.end())
                break;
            corpRole::const_iterator itr = cItr->second.begin();
            while (itr != cItr->second.end()) {
                cVec.push_back(itr->first);
                ++itr;
            }
        } break;
    };

    for (auto cur : cVec) {
        PyIncRef(payload);
        cur->SendNotification( notifyType, idType, &payload, seq );
    }

    PyDecRef( payload );
}

// updated.  so much better this way.
void EntityList::Multicast(const char* notifyType, const char* idType, PyTuple** in_payload, const MulticastTarget &mcset, bool seq)
{
    // consume payload
    PyTuple* payload = *in_payload;
    in_payload = nullptr;

    if (!mcset.characters.empty())
        for (auto cur : mcset.characters) {
            std::map<uint32, Client*>::iterator itr = m_players.find(cur);
            if ( itr != m_players.end()) {
                PyIncRef(payload);
                itr->second->SendNotification( notifyType, idType, &payload, seq );
            }
        }

    if (!mcset.locations.empty()) {
        SystemManager* pSysMgr(nullptr);
        std::vector<Client*> cVec;
        cVec.clear();
        for (auto cur : mcset.locations) {
            if (sDataMgr.IsStation(cur)) {
                GetStationGuestList(cur, cVec);
            } else if (sDataMgr.IsSolarSystem(cur)) {
                pSysMgr = FindOrBootSystem(cur);
                if (pSysMgr == nullptr)
                    continue;
                pSysMgr->GetClientList(cVec);
            } else {
                sLog.Error("EntityList::Multicast 2", "location %u is neither station nor system", cur);
                EvE::traceStack();
            }
        }
        for (auto cur : cVec) {
            PyIncRef(payload);
            cur->SendNotification( notifyType, idType, &payload, seq );
        }
    }

    // this will need list of interested parties from corp.  update this call to use CorpNotify() where possible.
    if (!mcset.corporations.empty()) {
        for (auto cur : mcset.corporations) {
            std::map<uint32, corpRole>::const_iterator cItr = m_corpMembers.find(cur);
            if (cItr == m_corpMembers.end())
                continue;
            corpRole::const_iterator itr = cItr->second.begin();
            while (itr != cItr->second.end()) {
                PyIncRef(payload);
                itr->first->SendNotification( notifyType, idType, &payload, seq );
                ++itr;
            }
        }
    }

    PyDecRef( payload );
}

void EntityList::Multicast(const character_set &cset, const char* notifyType, const char* idType, PyTuple** in_payload, bool seq) const
{
    // consume payload
    PyTuple* payload = *in_payload;
    in_payload = nullptr;

    std::map<uint32, Client*>::const_iterator itr = m_players.begin();
    for (auto cur : cset) {
        itr = m_players.find(cur);
        if (itr != m_players.end()) {
            PyIncRef(payload);
            itr->second->SendNotification(notifyType, idType, &payload, seq);
        }
    }
    PyDecRef( payload );
}

void EntityList::Unicast(uint32 charID, const char* notifyType, const char* idType, PyTuple** payload, bool seq) {
    Client* pClient = FindClientByCharID(charID);
    if (pClient != nullptr)
        pClient->SendNotification( notifyType, idType, payload, seq );
}

/** @todo @note NOTE: TODO: HACK: the Find* methods below can get very expensive for many players */

//used by gmCommands....i dont like this one either....but at least it's use will be seldom
Client* EntityList::FindClientByName(const char* name) const {
    for (auto cur : m_players) {
        CharacterRef cRef = cur.second->GetChar();
        if (cRef.get() != nullptr)
            if (strcmp(cRef->name(), name) == 0)
                return cur.second;
    }
    return nullptr;
}

/** @todo  this needs more work.  hacked for now...  */
void EntityList::RegisterSID(int64 &sessionID) {
    /*  this whole method is just made up...eventually it will return a unique long long */
    /* max for int64 = 9223372036854775807 */
    std::set<int64>::iterator itr = m_sessions.find(sessionID);
    std::pair<std::set<int64>::iterator, bool > test;
    if (itr == m_sessions.end())
        test = m_sessions.insert(sessionID);
    if (test.second)
        return;
}

void EntityList::RemoveSID ( int64 sessionID ) {
    m_sessions.erase(sessionID);
}

uint32 EntityList::CreateNotification(uint32 receiverID, uint8 typeID, uint32 senderID, PyDict* data)
{
    // insert notification record
    DBerror err;
    uint32 notifyID = 0;
    sDatabase.RunQueryLID(err, notifyID,
        "INSERT INTO notification (typeID, senderID, receiverID, processed, created, deleted)"
        " VALUES (%u, %u, %u, 0, %lli, 0)",
        typeID, senderID, receiverID, (int64)GetFileTimeNow());

    // marshal data dict and store in notificationText
    if (notifyID > 0 and data != nullptr) {
        Buffer dataBuf;
        if (Marshal(data, dataBuf)) {
            std::string dataStr(dataBuf.begin<char>(), dataBuf.end<char>());
            std::string dataEscaped;
            sDatabase.DoEscapeString(dataEscaped, dataStr);
            sDatabase.RunQuery(err,
                "INSERT INTO notificationText (notificationID, data) VALUES (%u, '%s')",
                notifyID, dataEscaped.c_str());
        }
    }

    // send live push if receiver is online
    Client* pClient = FindClientByCharID(receiverID);
    if (pClient != nullptr) {
        OnNotify onn;
        onn.notifyID = notifyID;
        onn.typeID   = typeID;
        onn.senderID = senderID;
        onn.created  = GetFileTimeNow();
        onn.data     = data;
        PyTuple* payload = new PyTuple(1);
        payload->SetItem(0, onn.Encode());
        pClient->SendNotification("OnNotificationReceived", "clientID", payload, false);
    }

    return notifyID;
}
