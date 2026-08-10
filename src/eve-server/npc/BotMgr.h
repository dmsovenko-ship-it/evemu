#ifndef EVEMU_PLAYERBOT_BOTMGR_H_
#define EVEMU_PLAYERBOT_BOTMGR_H_

#include "eve-compat.h"
#include "eve-common.h"
#include "utils/Singleton.h"
#include <unordered_map>
#include <map>
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

    // Hook called by LSCChannel when a message is sent in a system channel.
    // Lets simulated players in that system react (DeepSeek replies, smalltalk).
    void HandleLocalMessage(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Called when a player writes in a channel where bots recently spoke — records
    // a chat reply for self-learning (positive chat reinforcement).
    void HandleLocalReply(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message);
    // Top up bots in a freshly-loaded system that has real players.
    void PopulateSystem(SystemManager* pSystem);

private:
    void SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID);
    // Spawn a bot in `origin` and make it fly to `destSystem`'s gate (arrival
    // through the gate, visible warp). Used by PopulateSystem.
    void SpawnBotArriving(SystemManager* origin, uint32 destSystem);
    void ReapBots(SystemManager* pSystem);
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
    void PlaceBotOrder(PlayerBot* bot);
    // Docked bots at a station, for the station "pilots at station" (GetGuests).
    struct GuestInfo { uint32 charID, corpID, allianceID, warFactionID; };
    void GetDockedAtStation(uint32 stationID, std::vector<GuestInfo>& out) const;
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
        time_t undockAt;   // when to undock (0 = already waiting)
    };
    void ProcessDocking();   // manage dock/undock cycle each tick

    bool m_initalized;
    uint32 m_botCounter;    // unique bot instance id generator
    std::map<int32, time_t> m_lastChatReply;   // channelID -> last DeepSeek reply time (throttle)
    std::map<uint32, std::vector<DockedBot>> m_docked;   // systemID -> docked bots
    std::map<uint32, uint32> m_systemTarget;   // systemID -> fixed bot target (live-server feel)
    std::map<uint32, time_t> m_lastPopulate;   // systemID -> last bot spawn time (gradual fill)
};

//Singleton
#define sBotMgr \
( BotMgr::get() )

#endif  // EVEMU_PLAYERBOT_BOTMGR_H_
