#ifndef EVEMU_PLAYERBOT_BOTMGR_H_
#define EVEMU_PLAYERBOT_BOTMGR_H_

#include "eve-compat.h"
#include "eve-common.h"
#include "utils/Singleton.h"
#include <unordered_map>
#include <map>
#include <set>
#include <deque>
#include <atomic>
#include <ctime>

class SystemManager;
class PlayerBot;
class SystemEntity;

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

    // Boot-time pilot-pool trim: if the persistent pool exceeded MaxTotalPilots
    // (e.g. after a cleanup+respawn race), delete the NEWEST characters beyond
    // the cap (plus their memory/portrait/mail/killmail/skill rows) so the
    // spawner reuses the stable capped pool instead of growing it.
    static void TrimPilotPool();

    // Procedural portrait: writes a deterministic randomised 512x512 PNG "pilot
    // bust" to `path` (seed = serverCharID → same pilot always renders the same
    // face). Used by genportrait standalone mode (called from a forked child in
    // FetchPortraitAsync when ESI download is unavailable — image.evetech.net
    // is blocked from RU); ensures a bot is never left without a portrait.
    static bool GeneratePortraitPNG(const std::string& path, uint32 seed);

    // Hook called by LSCChannel when a message is sent in a system channel.
    // Lets simulated players in that system react (DeepSeek replies, smalltalk).
    void HandleLocalMessage(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Called when a player writes in a channel where bots recently spoke — records
    // a chat reply for self-learning (positive chat reinforcement).
    void HandleLocalReply(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Top up bots in a freshly-loaded system that has real players.
    void PopulateSystem(SystemManager* pSystem);
    // Load the always-on system set (trade hubs from botTradeHubs + the
    // AlwaysOnSystems config list), boot them and mark them persistent so they
    // stay loaded and keep a baseline bot population with no player online.
    void EnsureAlwaysOnSystems();

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
    void MaterializeBotFit(InventoryItemRef shipRef, uint32 charID, const std::string& fitJson, uint32 buyStationID = 0);
    // Re-buy a killed bot's fit on the open market (see BotBuyStock) with real
    // ISK, upgrading each module as far as the pilot's skill tier allows. Returns
    // a re-serialised fit JSON of what was actually bought (empty if nothing).
    std::string ResupplyBotFit(uint32 charID, uint32 stationID, uint8 skillTier, const std::string& fitJson);
    // The T1→meta→T2 ladder for one module, best-first and filtered by the
    // pilot's tier (see implementation for the tier→tech gating).
    std::vector<uint32> FitUpgradePath(uint32 baseType, uint8 skillTier);
    // After fitting: load charges/ammo for the hull's weapon (T1 for rookies, T2
    // once the bot's skill tier is high enough) and put a small profession-typical
    // cargo in the hold — a real pilot has ammo and a hold that matches their job.
    void MaterializeShipLoad(InventoryItemRef shipRef, uint32 charID, uint8 profession, uint8 skillTier);
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
    void PayMissionReward(PlayerBot* bot);
    // Market orders/contracts, placed at a specific station (from a docked bot).
    // The PlayerBot* overloads are for space bots (kept for compat); the explicit
    // versions take a sysID/station so docked traders can work the market.
    void PlaceBotOrder(PlayerBot* bot);
    void PlaceBotBuyOrder(PlayerBot* bot);
    void PlaceBotCourierContract(PlayerBot* bot);
    void PlaceBotOrderAt(uint32 sysID, uint32 charID, uint32 corpID);
    void PlaceBotBuyOrderAt(uint32 sysID, uint32 charID, uint8 profession);
    uint32 PlaceBotCourierContractAt(uint32 sysID, uint32 charID, uint32 corpID);
    // Stage-2 living goods: a bot with REAL stock sitting in its station hangar
    // (ore/salvage/faction loot deposited by miners/ratters) packs that physical
    // cargo into a public courier contract to the trade hub. The goods are locked
    // into the contract (owner -> contract), a courier hauls them there, and
    // CompleteContract delivers them to the issuer's hangar at the hub — real
    // goods physically travel between stations. Returns the contract id, or 0.
    uint32 PlaceStockCourierContractAt(uint32 sysID, uint32 stationID, uint32 charID, uint32 corpID);
    // Trader lists real stock as a public item-exchange (auction=false) or
    // auction (auction=true) contract at its station. Items are locked into the
    // contract and a link is announced in local when a player is present.
    // Returns the contract id, or 0.
    uint32 PlaceBotItemContractAt(uint32 sysID, uint32 stationID, uint32 charID, uint32 corpID, bool auction);
    // Post a clickable contract link in the local chat of its system, but only
    // if a real player is there (throttled per system so it never spams).
    void AnnounceBotContract(uint32 sysID, uint32 contractId, const std::string& title,
                             uint32 charID, const std::string& name, uint32 corpID);
    // A bot docked at the trade hub SELLS its real stock into the best resting
    // buy order per type (closing the ISK loop: ore/faction loot hauled to Jita
    // actually becomes ISK). Returns total ISK received.
    double SellStockAtHub(uint32 sysID, uint32 stationID, uint32 charID);
    // Courier bots pick up player courier contracts that have been sitting
    // unaccepted; they haul the cargo to the destination station.
    void ProcessPlayerContracts();
    // A free courier bot in `systemID`, or across any loaded system when 0.
    PlayerBot* FindFreeCourier(uint32 systemID);
    // Complete courier hauls that reached their destination but never docked.
    void ProcessHaulDeliveries();
    // Market self-learning (stage-1 economy): a docked trader reads its station's
    // order book and either captures a crossing spread (real arbitrage fills via
    // MarketMgr::BotArbitrageFill) or quotes tighter than the current best bid/
    // ask (market-making). Its remembered tradeProfit (BotMemory) tunes how bold
    // it is: after profits it quotes tight and chases volume; after losses it
    // widens its required margin and trades less.
    // sysID/stationID = where the bot is docked; db carries its char/corp/prof.
    struct DockedBot;   // full definition below (member methods take it by ref)
    void ProcessDockedTraderEconomy(uint32 sysID, uint32 stationID, const DockedBot& db);
    // Producer/builder: runs the full recursive manufacturing chain (invTypeMaterials),
    // buys missing inputs on the market, then ships/sells the output like other producers.
    void ProcessDockedIndustrialEconomy(uint32 sysID, uint32 stationID, const DockedBot& db);
    // Run the bot's planetary colony schematic chain (P1->P2->P3->P4).
    void ProcessIndustrialistPI(uint32 sysID, uint32 stationID, const DockedBot& db);
    // Moon-reaction POS production: harvesters accumulate raw Moon Materials,
    // simple/complex reactors convert them to intermediates and composites
    // (advanced materials) — closes the industrial chain at the tower.
    void ProcessMoonPOSProduction(uint32 sysID, uint32 stationID, const DockedBot& db);
    // Anchor a corp-owned Customs Office at the bot's colony planet (idempotent).
    void DeployBotCustomsOffice(SystemManager* sysMgr, uint32 charID, uint32 corpID, uint32 planetID);
    // Deploy a POS (Control Tower + Assembly Array + Silo) at a moon in the
    // system for this producer corp. No-op if the corp already has one there.
    void DeployBotPOS(SystemManager* sysMgr, uint32 charID, uint32 corpID);
    // Fill a bot POS to its doctrine within the tower's CPU/Powergrid budget:
    // production first, then shield hardeners (resists), tackle and small/medium
    // weapon batteries. Modules already anchored (same corp, inside the field)
    // count against the budget, so re-running only tops the POS up. `installValue`
    // (optional) accumulates the base price of newly anchored modules (billing).
    void FitBotPOSModules(SystemManager* sysMgr, uint32 corpID, uint32 towerItemID,
                          const GPoint& pos, double R, double* installValue = nullptr);
    // Periodic sweep: top every loaded bot POS up to its doctrine (fills free
    // CPU/grid with resists and small guns; also re-fits POSs built by older code).
    void EnsureBotPOSFittings();
    // Remove module surplus from bot POSes (leftovers from the pre-idempotent
    // sweep: dozens of hardeners/dampeners). Keeps the doctrine caps.
    void TrimBotPOSModules();
    // Spawn same-corp guard pilots at a POS. Two arrival models: "login at a
    // station then warp in" or "login at the POS" (warp-in out of nowhere).
    // Null-sec mostly the latter, high-sec 50/50.
    void SpawnPosGuards(SystemManager* sysMgr, uint32 corpID, const GPoint& pos);
    // POS guards assist the tower operator's manual target (focus fire).
    void ProcessPosGuards();
    // A courier reached the destination system — complete its accepted contract
    // (reward ISK + cargo placed at the destination station).
    void CompleteContract(uint32 charID, uint32 destSystem);
    // Recover courier contracts whose BOT acceptor stalled (it was reaped while
    // transiting an empty system, so it never "arrived"): complete them so the
    // contract market keeps flowing instead of clogging at status=1 forever.
    void ReapStaleContracts();
    // Docked traders buy up open item-exchange contracts at their station (without
    // a buyer, WTS contracts pile up and the contract market looks dead).
    void ProcessContractBuyers();
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

    // Smalltalk lines live in the botSmalltalk table (seeded by migration,
    // topped up by DeepSeek).  Pools are cached per profession (255 = the
    // "under attack" combat pool) with a short TTL; picking weights the
    // least-used lines, so the base actually rotates.
    void LoadSmalltalkPool(uint8 pool);
    // Ask DeepSeek for fresh smalltalk lines for one pool (rotating), insert the
    // new ones.  Runs every 30 min when ChatEnabled + DeepSeekKey are set.
    void ExpandSmalltalkPool();
    std::map<uint8, std::pair<time_t, std::vector<std::string>>> m_smalltalk;

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
    std::set<uint32> m_alwaysOn;               // systems kept loaded + populated 24/7
    bool m_alwaysOnLoaded = false;
    std::map<uint32, time_t> m_lastPopulate;   // systemID -> last bot spawn time (gradual fill)
    std::map<int32, time_t> m_lastSmalltalk;   // channelID -> last bot-to-bot chatter time
    std::map<uint32, time_t> m_lastContractLink;   // systemID -> last contract link posted in local
    std::map<uint32, time_t> m_lastTrade;      // charID -> last market order time (throttle)

    // Physical courier hauls: a courier accepted a courier contract and flies it
    // gate-to-gate to the destination system (bot is deleted/re-spawned at each
    // gate like every traveller). The map is keyed by courier charID so the run
    // survives the per-hop delete+respawn.
    struct CourierHaul {
        uint32 contractID  = 0;
        uint32 endSys      = 0;     // destination system
        uint32 endStation  = 0;     // destination station
        std::vector<uint32> route;  // remaining systems to cross (front = next hop)
        time_t arrivedAt   = 0;     // when the courier reached endSys (0 = en route)
    };
    std::map<uint32, CourierHaul> m_hauls;   // courier charID -> active haul

    // POS supply run: a docked industrialist physically flies to its corp's tower
    // carrying fuel / ammo / reaction materials (and a market-bought BPC) and
    // unloads into the tower's cargo, then heads back to the station.  Makes POS
    // upkeep visible logistics instead of abstract minting.
    struct PosSupplyRun {
        uint32 towerID    = 0;
        uint32 stationID  = 0;
        uint32 sysID      = 0;
        uint32 corpID     = 0;
        uint32 allianceID = 0;
        std::string name;
        uint8  phase      = 0;   // 0 undock+load, 1 warp to POS, 2 unload, 3 return
        int64  phaseAt    = 0;
        int64  warpAt     = 0;
    };
    void StartPosSupplyRun(const DockedBot& db, uint32 sysID, uint32 stationID);
    void ProcessPosSupplyRuns();
    std::map<uint32, PosSupplyRun> m_posSupply;   // charID -> run
    std::map<uint32, int64> m_lastPosSupply;      // charID -> last run start (throttle)

    // POS guard pilots are EXCLUSIVE: the normal population/travel logic must not
    // spawn a second copy of a guard, nor warp it away from its tower. Populated
    // when SpawnPosGuards assigns a pilot; pruned in ProcessPosGuards when the
    // guard SE is gone.
    std::set<uint32> m_guardPilots;   // charID -> reserved as a tower guard

    // Highsec outlaw gankers: CONCORD answers a few seconds after the gank, the
    // window scaled by system security (Crucible: ~6s at 1.0, ~19s at 0.5). The
    // ganker is destroyed by a spawned CONCORD ship (proper killmail attribution);
    // the temp CONCORD ships are despawned after a while.
    void ScheduleConcordGank(uint32 charID, uint32 sysID);
    void ProcessOutlawConcord();
    std::map<uint32, int64> m_concordGankAt;     // outlaw charID -> CONCORD strike time
    struct ConcordTemp { uint32 sysID; uint32 seID; int64 at; };   // spawned CONCORD ships
    std::vector<ConcordTemp> m_concordTemp;      // store IDs (system may unload)

    // System adjacency cache (lazy, loaded from mapSolarSystemJumps).
    static std::vector<uint32> GetAdjacentSystems(uint32 systemID);
    // BFS shortest path from..to over the jump graph; true if a route exists.
    bool ComputeHaulRoute(uint32 fromSys, uint32 toSys, std::vector<uint32>& out);
};

//Singleton
#define sBotMgr \
( BotMgr::get() )

// Enriched "top kills" block shared by the daily TG digest and the /topkills
// Telegram command. sinceSql is a SQL boolean restricting the window ("1" =
// all time). Returns empty when there are no rows in range.
std::string BuildKillDigestText(int limit, const std::string& sinceSql);
// Compact ISK formatting: 30000 → "30.0k", 1200000 → "1.20m", 3.4e9 → "3.40b".
std::string HumanizeIsk(double v);

#endif  // EVEMU_PLAYERBOT_BOTMGR_H_
