#ifndef EVEMU_PLAYERBOT_BOTMGR_H_
#define EVEMU_PLAYERBOT_BOTMGR_H_

#include "eve-compat.h"
#include "eve-common.h"
#include "utils/Singleton.h"
#include <unordered_map>
#include <map>
#include <deque>
#include <atomic>
#include <ctime>

class SystemManager;
class PlayerBot;

/**
 * @brief Manages simulated players (AI pilots) that populate active systems
 * like a live server.
 *
 * On the 1Hz tic it walks every loaded system, tops up each one to
 * sConfig.playerBots.MaxPerSystem bots (only for systems that have real
 * players in them), and ticks every bot so it can make decisions.
 *
 * A bot's "legend" (name/corp/alliance/characterID) comes from the agents
 * table so portraits and names look like real pilots.
 *
 * @author bot-infrastructure
 */
class BotMgr
: public Singleton<BotMgr>
{
public:
    BotMgr();
    ~BotMgr() { /* do nothing here */ }

    int Initialize();
    void Process();     // called from EntityList on the 1Hz tic

    // Boot sweep for the portal's "147 ships with 3 online" problem: after an
    // unclean shutdown, chelobot hulls / transient NPC ships and drones stay
    // behind in the entity table as in-space garbage (nobody is flying them).
    // At Initialize nothing is in space yet, so every leftover is an orphan:
    //   * corp-owned ships (ownerID = corporation) not listed as any character's
    //     active ship — removed with their fitted modules/charges,
    //   * drones whose pilot is not actively flying in that system (docked or
    //     logged off) — a player on a station never has legal drones in space.
    // Stale chelobot session rows (docked bots keep online=1 + a dangling
    // shipID) are reset first so crash leftovers are caught too.
    static void CleanupOrphanedSpaceItems();

    // Hook called by LSCChannel when a message is sent in a system channel.
    // Lets simulated players in that system react (DeepSeek replies, smalltalk).
    void HandleLocalMessage(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Called when a player writes in a channel where bots recently spoke — records
    // a chat reply for self-learning (positive chat reinforcement).
    void HandleLocalReply(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Top up bots in a freshly-loaded system that has real players.
    void PopulateSystem(SystemManager* pSystem);

    // Docked bots at a station, for the station "pilots at station" (GetGuests).
    struct GuestInfo { uint32 charID, corpID, allianceID, warFactionID; };
    void GetDockedAtStation(uint32 stationID, std::vector<GuestInfo>& out) const;

    // Counts of simulated players for the portal /server status "online" figure.
    // Chelobots look like real pilots, so the world's population = real clients
    // + chelobots flying in space + chelobots docked at stations.
    // Counts are refreshed once per BotMgr tic (game thread) into atomics so the
    // API thread can read them safely (no cross-thread iteration of live SEs).
    uint32 CountActiveBots() const   { return m_activeBotCount.load(); }   // in-space chelobots
    uint32 GetDockedBotCount() const { return m_dockedBotCount.load(); }   // chelobots docked

    // Bot chat replies are QUEUED (not recursed): SendBotMessage feeds the queue,
    // BotMgr drains one per tic so a bot<-bot conversation advances without
    // overflowing the stack (SIGSEGV from nested HandleLocalMessage/SendBotMessage).
    struct PendingBotReply { int32 channelID; uint32 charID; std::string name; std::string message; };
    void QueueBotReply(const PendingBotReply& r)   { m_pendingBotReplies.push_back(r); }
    // Remember a bot line per channel (with a time cap) so bots don't repeat a
    // phrase that was just said by another bot in the same channel.
    void RecordChannelPhrase(int32 channelID, uint32 charID, const std::string& phrase);

    // Compose a profession-flavoured, situation-aware local line for a chelobot
    // (what it is ACTUALLY doing: mining/ratting/hauling, or under attack). Used
    // by ProcessBotSmalltalk so bots never say off-topic things (a miner doesn't
    // shout "pvp?" from a belt).
    std::string BuildBotSmalltalkLine(PlayerBot* a, PlayerBot* b, SystemManager* pSystem);

    // Primary trade hub system (Jita by default), from botTradeHubs. 0 if none.
    uint32 GetTradeHubSystem() const;
    bool IsTradeHub(uint32 systemID) const;

private:
    void SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID, bool arrivedViaGate = false);
    // Materialize a killmail fit (JSON array of module typeIDs) into a bot's ship:
    // spawns each module as a real item in the correct slot (hi/mid/low/rig per its
    // dogma power effect), so the client shows the real fit and the wreck drops
    // actual module loot. Skipped when the ship has no slots or fit is empty.
    void MaterializeBotFit(InventoryItemRef shipRef, uint32 charID, const std::string& fitJson);
    // Download the bot's ESI portrait into the image cache on spawn, so the client
    // sees a face immediately (no cron lag). Runs curl in a forked child so the
    // game loop isn't blocked. Path: <imageDir>/Character/<serverCharID>_512.jpg.
    static void FetchPortraitAsync(uint32 serverCharID, uint32 eveCharID);
    // Spawn a bot in `origin` and make it fly to `destSystem`'s gate (arrival
    // through the gate, visible warp). Used by PopulateSystem.
    void SpawnBotArriving(SystemManager* origin, uint32 destSystem);
    void ReapBots(SystemManager* pSystem);
    void RefreshOnlineCount();   // recompute active/docked counts (game thread, once/tic)
    // Realistic corp distribution: one "main" corp holds most bots, 2-3 smaller
    // corps the rest (like live EVE). Picks a corpID for a new bot.
    // requireAlliance = only corps inside an alliance (for PvP war corps that
    // skirmish for sovereignty).
    uint32 PickCorp(uint32& allianceID, bool requireAlliance=false);
    // Lets a few bots head for a gate / travel to a neighbouring system each tick.
    void ProcessTravel();
    // Random system reachable by gate from systemID (mapSolarSystemJumps), 0 if none.
    uint32 GetRandomAdjacentSystem(uint32 systemID);
    // Random ship name (players rename their hulls to arbitrary words/codes).
    static std::string MakeRandomShipName();
    // Random corp name/ticker for a bot-founded corporation.
    static std::string MakeCorpName();
    static std::string MakeTicker();
    // Experienced leader bots can found their own corporation (start in NPC corps,
    // later branch off). Transfers the bot to the new corp as CEO.
    void MaybeFoundCorp(PlayerBot* bot);
    // A practised founder unites several bot-founded corps (same profession or
    // location) into an alliance.
    void MaybeFormAlliance(PlayerBot* bot);
    static std::string MakeAllianceName();
    // Economy: bots earn ISK from profession activity, pay corp tax into the
    // corp wallet, and trader bots place market orders in their own name.
    void ProcessEconomy(PlayerBot* bot);
    void PayCorpTax(PlayerBot* bot);
    // Market orders/contracts, placed at a specific station (from a docked bot).
    // The PlayerBot* overloads are for space bots (kept for compat); the explicit
    // versions take a sysID/station so docked traders can work the market.
    void PlaceBotOrder(PlayerBot* bot);
    void PlaceBotBuyOrder(PlayerBot* bot);
    void PlaceBotCourierContract(PlayerBot* bot);
    void PlaceBotOrderAt(uint32 sysID, uint32 charID, uint32 corpID);
    void PlaceBotBuyOrderAt(uint32 sysID, uint32 charID, uint8 profession);
    void PlaceBotCourierContractAt(uint32 sysID, uint32 charID, uint32 corpID);
    // Stage-2 living goods: a bot with REAL stock sitting in its station hangar
    // (ore/salvage/faction loot deposited by miners/ratters) packs that physical
    // cargo into a public courier contract to the trade hub. The goods are locked
    // into the contract (owner -> contract), a courier hauls them there, and
    // CompleteContract delivers them to the issuer's hangar at the hub — real
    // goods physically travel between stations. Returns the contract id, or 0.
    uint32 PlaceStockCourierContractAt(uint32 sysID, uint32 stationID, uint32 charID, uint32 corpID);
    // A bot docked at the trade hub SELLS its real stock into the best resting
    // buy order per type (closing the ISK loop: ore/faction loot hauled to Jita
    // actually becomes ISK). Returns total ISK received.
    double SellStockAtHub(uint32 sysID, uint32 stationID, uint32 charID);
    // Courier bots pick up player courier contracts that have been sitting
    // unaccepted; they haul the cargo to the destination station.
    void ProcessPlayerContracts();
    // Market self-learning (stage-1 economy): a docked trader reads its station's
    // order book and either captures a crossing spread (real arbitrage fills via
    // MarketMgr::BotArbitrageFill) or quotes tighter than the current best bid/
    // ask (market-making). Its remembered tradeProfit (BotMemory) tunes how bold
    // it is: after profits it quotes tight and chases volume; after losses it
    // widens its required margin and trades less.
    // sysID/stationID = where the bot is docked; db carries its char/corp/prof.
    struct DockedBot;   // full definition below (member methods take it by ref)
    void ProcessDockedTraderEconomy(uint32 sysID, uint32 stationID, const DockedBot& db);
    // A courier reached the destination system — complete its accepted contract
    // (reward ISK + cargo placed at the destination station).
    void CompleteContract(uint32 charID, uint32 destSystem);
    // Deterministic real-EVE-style corp logo from a seed id: slot 0 = graphicID,
    // 1-3 = colors (0xRRGGBB), 4-6 = shapes. Same seed → same logo.
    static int64 MakeCorpLogo(uint32 seed, uint8 slot);

    // Docked bots: present in local but not flying in space (no SE). They are
    // "at the station". Occasionally undock (spawn at the station) and leave.
    struct DockedBot {
        uint32 charID;
        std::string name;
        uint32 corpID;
        uint32 allianceID;
        uint8  profession;   // PlayerBot::BotProfession while docked
        uint32 stationID;    // station the bot is docked at (0 = any/unknown)
        time_t undockAt;   // when to undock (0 = already waiting)
    };
    void ProcessDocking();   // manage dock/undock cycle each tick
    // Docked traders work the market FROM THE STATION (that's where a market
    // order lives). Run each tick: docked traders place sell/buy orders and the
    // occasional courier contract at their station; docked producers bid for raw
    // materials. Space bots don't trade — they're flying, not on the market.
    void ProcessDockedEconomy();
    // Bots occasionally chat among themselves in local (rare, so it doesn't
    // spam). Makes the channel feel alive without DeepSeek calls.
    void ProcessBotSmalltalk();
    // Drain queued bot chat replies (one per tic) so bot<-bot conversations
    // advance without recursing the call stack.
    void ProcessBotReplies();

    bool m_initalized;
    uint32 m_botCounter;    // unique bot instance id generator
    std::atomic<uint32> m_activeBotCount{ 0 };   // refreshed each tic (game thread)
    std::atomic<uint32> m_dockedBotCount{ 0 };   // refreshed each tic (game thread)
    std::map<int32, time_t> m_lastChatReply;   // channelID -> last DeepSeek reply time (throttle)
    struct BotPhrase { uint32 charID; std::string phrase; time_t when; };
    std::map<int32, BotPhrase> m_lastBotPhrase;   // channelID -> last bot line (for learning replies)
    std::map<int32, std::deque<BotPhrase>> m_channelPhrases;   // channelID -> recent bot lines (no-repeat guard)
    std::map<int32, uint32> m_botChainDepth;   // channelID -> consecutive bot-bot replies (loop breaker)
    std::vector<PendingBotReply> m_pendingBotReplies;
    std::map<uint32, std::vector<DockedBot>> m_docked;   // systemID -> docked bots
    std::map<uint32, uint32> m_systemTarget;   // systemID -> fixed bot target (live-server feel)
    std::map<uint32, time_t> m_lastPopulate;   // systemID -> last bot spawn time (gradual fill)
    std::map<int32, time_t> m_lastSmalltalk;   // channelID -> last bot-to-bot chatter time
    std::map<uint32, time_t> m_lastTrade;      // charID -> last market order time (throttle)
};

//Singleton
#define sBotMgr \
( BotMgr::get() )

#endif  // EVEMU_PLAYERBOT_BOTMGR_H_
