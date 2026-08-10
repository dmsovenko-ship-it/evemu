#include "eve-server.h"
#include "npc/BotMgr.h"
#include "npc/PlayerBot.h"
#include "npc/NPC.h"
#include "EntityList.h"
#include "system/SystemManager.h"

/*
 * @file BotMgr.cpp
 *
 * Simulated players. Populates active systems (systems with real pilots in
 * them) with up to sConfig.playerBots.MaxPerSystem AI pilots, and ticks them
 * on the server's 1Hz tic. Pilot legends (name/corp) come from the NPC agent
 * database so they look like real characters.
 */

BotMgr::BotMgr()
: m_initalized(false),
  m_botCounter(0)
{
}

int BotMgr::Initialize()
{
    m_initalized = true;
    if (sConfig.playerBots.Enabled) {
        sLog.Green("      BotMgr", "Simulated players ENABLED (max %u per system, chat %u%%, skill %u-%u).",
                   sConfig.playerBots.MaxPerSystem, sConfig.playerBots.ChatChance,
                   sConfig.playerBots.MinSkillLevel, sConfig.playerBots.MaxSkillLevel);
    } else {
        sLog.Green("      BotMgr", "Simulated players DISABLED.");
    }
    return 0;
}

void BotMgr::Process()
{
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;

    // Walk every loaded system; top up any that has real players in it.
    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr)
            continue;
        if (pSystem->PlayerCount() < 1)
            continue;   // only simulate where real players are present

        PopulateSystem(pSystem);
    }
}

void BotMgr::PopulateSystem(SystemManager* pSystem)
{
    if (pSystem == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (sConfig.playerBots.MaxPerSystem == 0)
        return;

    // Count existing bots in this system.
    uint32 botCount = 0;
    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se != nullptr && se->GetNPCSE() != nullptr
            && dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
            ++botCount;
    }

    uint32 target = sConfig.playerBots.MaxPerSystem;
    // A "live server" feel: usually not the cap. Aim for a random 60-100% of the cap.
    if (botCount == 0)
        target = (uint32)(sConfig.playerBots.MaxPerSystem * (0.6f + MakeRandomFloat() * 0.4f));
    if (botCount >= target)
        return;

    for (uint32 i = botCount; i < target; ++i)
        SpawnBot(pSystem, 0, "", 0, 0);
}

void BotMgr::SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID)
{
    // Pull a random agent identity (name + corp) from the agents DB.
    uint32 useCharID = charID;
    std::string useName = name;
    uint32 useCorpID = corpID;
    if (useName.empty()) {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT a.agentID, c.characterName, a.corporationID"
            " FROM agtAgents a"
            " JOIN chrNPCCharacters c ON a.agentID = c.characterID"
            " ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                useCharID = row.GetUInt(0);
                useName = row.GetText(1);
                useCorpID = row.GetUInt(2);
            }
        }
    }
    if (useName.empty())
        useName = "Pilot " + std::to_string(++m_botCounter);

    _log(BOT__MESSAGE, "BotMgr: spawning simulated player '%s' (char %u, corp %u) in system %u",
         useName.c_str(), useCharID, useCorpID, pSystem->GetID());

    // Pick a player-ish ship hull: cruiser or battlecruiser for a "professional".
    static const uint32 hullTypes[] = { 621, 633, 626, 613, 609, 597, 606, 601 };   // assorted T1 cruisers/BC
    uint32 hullType = hullTypes[MakeRandomInt(0, 7)];

    // Spawn at a gate — the bot "arrived through the gate from the neighbouring
    // system", matching a real pilot's travel story.
    GPoint pos;
    bool posSet = false;
    for (auto& [id, se] : pSystem->GetStaticEntities()) {
        if (se != nullptr && se->GetGateSE() != nullptr) {
            pos = se->GetPosition();
            posSet = true;
            break;
        }
    }
    if (!posSet) {
        for (auto& [id, se] : pSystem->GetStaticEntities()) {
            if (se != nullptr && se->GetStationSE() != nullptr) {
                pos = se->GetPosition();
                posSet = true;
                break;
            }
        }
    }
    if (!posSet)
        pos = GPoint(0, 0, 0);

    ItemData idata(hullType, useCorpID, pSystem->GetID(), flagNone, "", pos, useName.c_str());
    InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
    if (iRef.get() == nullptr) {
        _log(BOT__ERROR, "BotMgr: failed to spawn ship hull %u for bot.", hullType);
        return;
    }

    FactionData data = FactionData();
    data.corporationID = useCorpID;
    data.ownerID = useCorpID;
    data.factionID = 0;
    data.allianceID = allianceID;

    PlayerBot* bot = new PlayerBot(iRef, pSystem->GetServiceMgr(), pSystem, data,
                                   useCharID, useName, useCorpID, allianceID);
    if (bot == nullptr) {
        _log(BOT__ERROR, "BotMgr: failed to create PlayerBot.");
        return;
    }
    if (!bot->Load()) {
        _log(BOT__ERROR, "BotMgr: failed to Load PlayerBot, deleting.");
        bot->Delete();
        return;
    }
    // Professional pilot skill tier.
    uint8 skill = sConfig.playerBots.MinSkillLevel + MakeRandomInt(0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
    bot->GetAIMgr()->SetAmbush(false);   // bots are not ambushing rats
    bot->DestinyMgr()->SetPosition(pos);
    pSystem->AddNPC(bot);
}

void BotMgr::ReapBots(SystemManager* pSystem)
{
    // Remove bots from a system that no longer has real players.
    if (pSystem == nullptr)
        return;
    std::vector<PlayerBot*> toRemove;
    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se != nullptr && se->GetNPCSE() != nullptr
            && dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
            toRemove.push_back((PlayerBot*)se->GetNPCSE());
    }
    for (PlayerBot* bot : toRemove) {
        _log(BOT__MESSAGE, "BotMgr: reaping simulated player '%s' from system %u",
             bot->GetBotName().c_str(), pSystem->GetID());
        bot->Delete();
    }
}
