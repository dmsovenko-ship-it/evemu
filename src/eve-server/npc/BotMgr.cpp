#include "eve-server.h"
#include "npc/BotMgr.h"
#include "npc/PlayerBot.h"
#include "npc/NPCAI.h"
#include "npc/NPC.h"
#include "EntityList.h"
#include "system/SystemManager.h"
#include "corporation/CorporationDB.h"
#include "character/CharacterDB.h"

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

uint32 BotMgr::PickCorp(uint32& allianceID)
{
    // Realistic corp distribution: a few "main" corps hold most bots, a couple
    // of smaller ones the rest. Weight by existing member count so the biggest
    // corp naturally takes the largest share — just like live EVE. Bots are
    // persisted as real members, so the distribution self-organizes over time.
    DBQueryResult res;
    std::vector<std::pair<uint32,uint32>> corps;   // corpID, allianceID
    std::vector<uint32> weights;                    // members+1 per corp

    if (sDatabase.RunQuery(res,
        "SELECT c.corporationID, c.allianceID, COUNT(ch.characterID) AS members"
        " FROM crpCorporation c"
        " LEFT JOIN chrCharacters ch ON ch.corporationID = c.corporationID"
        " GROUP BY c.corporationID"
        " HAVING members >= 1"      // only corps that already have members
        " ORDER BY members DESC"
        " LIMIT 12"))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            uint32 corpID = row.GetUInt(0);
            uint32 allyID = row.GetUInt(1);
            uint32 members = row.GetUInt(2);
            corps.emplace_back(corpID, allyID);
            weights.push_back(members + 1);
        }
    }

    if (corps.empty()) {
        // No corps with members yet — fall back to any corp in the DB.
        if (sDatabase.RunQuery(res,
            "SELECT corporationID, allianceID FROM crpCorporation LIMIT 1")) {
            DBResultRow row;
            if (res.GetRow(row)) {
                allianceID = row.GetUInt(1);
                return row.GetUInt(0);
            }
        }
        allianceID = 0;
        return 0;
    }

    // Weighted random pick (weights = member counts).
    uint32 total = 0;
    for (uint32 w : weights) total += w;
    uint32 roll = MakeRandomInt(0, total - 1);
    for (size_t i = 0; i < corps.size(); ++i) {
        if (roll < weights[i]) {
            allianceID = corps[i].second;
            return corps[i].first;
        }
        roll -= weights[i];
    }
    allianceID = corps[corps.size()-1].second;
    return corps[corps.size()-1].first;
}

void BotMgr::SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID)
{
    // Pick a name from the agents DB if none given.
    std::string useName = name;
    if (useName.empty()) {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT c.characterName FROM chrNPCCharacters c ORDER BY RAND() LIMIT 1")) {
            DBResultRow row;
            if (res.GetRow(row))
                useName = row.GetText(0);
        }
    }
    if (useName.empty())
        useName = "Pilot " + std::to_string(++m_botCounter);

    // Corp: realistic distribution (main corp + a few smaller). If the caller
    // already supplied one (e.g. a persisted bot), keep it.
    uint32 useCorpID = corpID;
    uint32 useAllianceID = allianceID;
    if (useCorpID == 0)
        useCorpID = PickCorp(useAllianceID);
    if (useCorpID == 0) {
        _log(BOT__ERROR, "BotMgr: no corporation available for bot, skipping spawn.");
        return;
    }

    // Persist the bot as a REAL character (chrCharacters + skills + history) so
    // its legend and progress survive restarts. If a charID was supplied, reuse
    // the existing character; otherwise create one.
    uint32 useCharID = charID;
    if (useCharID == 0) {
        uint8 skillTier = sConfig.playerBots.MinSkillLevel +
            MakeRandomInt(0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
        useCharID = CharacterDB::CreateBotCharacter(useName, useCorpID, useAllianceID, skillTier);
        if (useCharID == 0) {
            _log(BOT__ERROR, "BotMgr: failed to create persisted bot character '%s'.", useName.c_str());
            return;
        }
    }

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
    data.allianceID = useAllianceID;

    PlayerBot* bot = new PlayerBot(iRef, pSystem->GetServiceMgr(), pSystem, data,
                                   useCharID, useName, useCorpID, useAllianceID);
    if (bot == nullptr) {
        _log(BOT__ERROR, "BotMgr: failed to create PlayerBot.");
        return;
    }
    if (!bot->Load()) {
        _log(BOT__ERROR, "BotMgr: failed to Load PlayerBot, deleting.");
        bot->Delete();
        return;
    }
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
