#ifndef EVEMU_PLAYERBOT_BOTMGR_H_
#define EVEMU_PLAYERBOT_BOTMGR_H_

#include "eve-compat.h"
#include "eve-common.h"
#include "utils/Singleton.h"
#include <unordered_map>

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

    // Top up bots in a freshly-loaded system that has real players.
    void PopulateSystem(SystemManager* pSystem);

private:
    void SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID);
    void ReapBots(SystemManager* pSystem);
    // Realistic corp distribution: one "main" corp holds most bots, 2-3 smaller
    // corps the rest (like live EVE). Picks a corpID for a new bot.
    uint32 PickCorp(uint32& allianceID);

    bool m_initalized;
    uint32 m_botCounter;    // unique bot instance id generator
};

//Singleton
#define sBotMgr \
( BotMgr::get() )

#endif  // EVEMU_PLAYERBOT_BOTMGR_H_
