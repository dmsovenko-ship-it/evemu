#include "eve-server.h"
#include "npc/BotMgr.h"
#include "npc/PlayerBot.h"
#include "npc/NPCAI.h"
#include "npc/NPC.h"
#include "npc/BotChat.h"
#include "EntityList.h"
#include "system/SystemManager.h"
#include "corporation/CorporationDB.h"
#include "character/CharacterDB.h"
#include "chat/LSCService.h"
#include "chat/LSCChannel.h"
#include "services/ServiceManager.h"

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

    // Occasionally let a bot travel to a neighbouring system (through a gate),
    // so the "population" moves around like a live server.
    ProcessTravel();

    // Manage docked bots: undock some each tick, dock others.
    ProcessDocking();
}

void BotMgr::PopulateSystem(SystemManager* pSystem)
{
    if (pSystem == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (sConfig.playerBots.MaxPerSystem == 0)
        return;

    // Count existing bots in this system (both in-space and docked).
    uint32 botCount = 0;
    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se != nullptr && se->GetNPCSE() != nullptr
            && dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
            ++botCount;
    }
    auto dockIt = m_docked.find(pSystem->GetID());
    if (dockIt != m_docked.end())
        botCount += (uint32)dockIt->second.size();

    // Fixed target per system (so we don't keep filling to the cap every tick).
    uint32 target = sConfig.playerBots.MaxPerSystem;
    auto tIt = m_systemTarget.find(pSystem->GetID());
    if (tIt == m_systemTarget.end()) {
        // Aim for a random 60-100% of the cap ONCE, then hold it — a live server
        // isn't at its cap every moment.
        target = (uint32)(sConfig.playerBots.MaxPerSystem * (0.6f + MakeRandomFloat() * 0.4f));
        m_systemTarget[pSystem->GetID()] = target;
    } else {
        target = tIt->second;
    }
    if (botCount >= target)
        return;

    // Spawn the missing bots in RANDOM neighbouring systems and have them fly
    // in through the gate — they "arrive from nearby systems" rather than
    // popping into existence at the player's location. Different bots come
    // from different gates (random adjacent systems), and their visible warp
    // is staggered by MarkForTravel's random 12-20s timer.
    for (uint32 i = botCount; i < target; ++i) {
        uint32 origin = GetRandomAdjacentSystem(pSystem->GetID());
        if (origin == 0) {
            // No map neighbours — spawn directly at a gate in this system.
            SpawnBot(pSystem, 0, "", 0, 0);
            continue;
        }
        SystemManager* originSys = sEntityList.FindOrBootSystem(origin);
        if (originSys == nullptr)
            continue;
        // Spawn there, then tell the bot to fly to THIS system's gate.
        // SpawnBot returns the bot via a helper; we mark it for arrival.
        SpawnBotArriving(originSys, pSystem->GetID());
    }
}

uint32 BotMgr::PickCorp(uint32& allianceID, bool requireAlliance /*false*/)
{
    // Realistic corp distribution: a few "main" corps hold most bots, a couple
    // of smaller ones the rest. Weight by existing member count so the biggest
    // corp naturally takes the largest share — just like live EVE. Bots are
    // persisted as real members, so the distribution self-organizes over time.
    // PvP war corps (requireAlliance) only get corps that are in an alliance.
    DBQueryResult res;
    std::vector<std::pair<uint32,uint32>> corps;   // corpID, allianceID
    std::vector<uint32> weights;                    // members+1 per corp

    std::string corpQuery = std::string(
        "SELECT c.corporationID, c.allianceID, COUNT(ch.characterID) AS members"
        " FROM crpCorporation c"
        " LEFT JOIN chrCharacters ch ON ch.corporationID = c.corporationID"
        " GROUP BY c.corporationID"
        " HAVING members >= 1")      // only corps that already have members
        + (requireAlliance ? " AND c.allianceID > 0" : "")
        + std::string(" ORDER BY members DESC")
        + " LIMIT 12";
    if (sDatabase.RunQuery(res, corpQuery.c_str()))
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
    // Pull a legend (name, corp, alliance, ship, fit) from real EVE killmail data.
    // This is the "believable backstory" — a real pilot who actually flew this
    // hull with this fit in live EVE. Falls back to a random agent name if the
    // killmail table is empty (first run before import_killmail_legends.py).
    std::string useName = name;
    uint32 useShipType = 0;
    uint32 useCharID = charID;
    uint32 useCorpID = corpID;
    uint32 useAllianceID = allianceID;
    std::string useFit;   // JSON array of module typeIDs (parsed later for fitting)

    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT character_id, character_name, corporation_id, alliance_id,"
            "       ship_type_id, fitted_item_ids"
            " FROM botKillmailLegends"
            " WHERE ship_type_id > 0 AND character_name != ''"
            " ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                useCharID = row.GetUInt(0);
                useName = row.GetText(1);
                useCorpID = row.GetUInt(2);
                useAllianceID = row.GetUInt(3);
                useShipType = row.GetUInt(4);
                const char* fit = row.GetText(5);
                if (fit != nullptr) useFit = fit;
            }
        }
    }
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

    // Decide the profession early so PvP war corps (hunters) are placed in an
    // alliance corp (required for claiming nullsec sovereignty).
    PlayerBot::BotProfession prof;
    {
        float p = MakeRandomFloat();
        if (p < 0.10f)
            prof = PlayerBot::BotProfession::Hunter;       // PvP pirates / war corps
        else if (p < 0.25f)
            prof = PlayerBot::BotProfession::RatHunter;    // peaceful PvE (red crosses only)
        else if (p < 0.50f)
            prof = PlayerBot::BotProfession::Miner;
        else if (p < 0.70f)
            prof = PlayerBot::BotProfession::Trader;
        else if (p < 0.85f)
            prof = PlayerBot::BotProfession::Courier;
        else
            prof = PlayerBot::BotProfession::Hacker;
    }

    // Corp: realistic distribution (main corp + a few smaller). If the killmail
    // legend already carries a corp, keep it; otherwise pick one by size.
    // Hunters (PvP war corps) only join corps inside an alliance.
    if (useCorpID == 0)
        useCorpID = PickCorp(useAllianceID, prof == PlayerBot::BotProfession::Hunter);
    if (useCorpID == 0) {
        _log(BOT__ERROR, "BotMgr: no corporation available for bot, skipping spawn.");
        return;
    }

    // Persist the bot as a REAL character (chrCharacters + portrait + skills +
    // history) so its legend and progress survive restarts. useCharID is the
    // killmail character id — pass it so the character row (and portrait) is
    // created under that identity. CreateBotCharacter reuses it if it exists.
    {
        uint8 skillTier = sConfig.playerBots.MinSkillLevel +
            MakeRandomInt(0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
        useCharID = CharacterDB::CreateBotCharacter(useName, useCorpID, useAllianceID, skillTier, useCharID);
        if (useCharID == 0) {
            _log(BOT__ERROR, "BotMgr: failed to create persisted bot character '%s'.", useName.c_str());
            return;
        }
    }

    // Ship hull from the killmail legend (real EVE hull) or a generic cruiser/BC.
    // Killmail hull types are from modern EVE — some don't exist in the server's
    // (Crucible-era) invTypes. Validate; fall back to a T1 cruiser/BC if invalid.
    uint32 hullType = useShipType;
    {
        Inv::TypeData tdata = Inv::TypeData();
        sDataMgr.GetType((uint16)hullType, tdata);
        bool valid = (hullType != 0) && (tdata.id == hullType);
        if (!valid) {
            static const uint32 hullTypes[] = { 621, 633, 626, 613, 609, 597, 606, 601 };   // assorted T1 cruisers/BC
            hullType = hullTypes[MakeRandomInt(0, 7)];
        }
    }

    _log(BOT__MESSAGE, "BotMgr: spawning simulated player '%s' (char %u, corp %u, ship %u, fit %zu items) in system %u",
         useName.c_str(), useCharID, useCorpID, hullType, useFit.size(), pSystem->GetID());

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

    // Assign a combat role: mostly fighters, a few logistics/support/commanders
    // so fights use the full arsenal (EWAR, remote reps, gang bonuses).
    {
        float r = MakeRandomFloat();
        if (r < 0.60f)
            bot->SetRole(PlayerBot::BotRole::Fighter);
        else if (r < 0.75f)
            bot->SetRole(PlayerBot::BotRole::Logistics);
        else if (r < 0.90f)
            bot->SetRole(PlayerBot::BotRole::Support);
        else
            bot->SetRole(PlayerBot::BotRole::Commander);
        _log(BOT__TRACE, "BotMgr: %s(%u) role = %u.", bot->GetBotName().c_str(), bot->GetBotCharID(), (uint8)bot->GetRole());
    }

    // Assign the profession decided earlier (before corp selection).
    bot->SetProfession(prof);
    _log(BOT__TRACE, "BotMgr: %s(%u) profession = %u.", bot->GetBotName().c_str(), bot->GetBotCharID(), (uint8)prof);

    // Join the system's local channel so the bot shows up in local chat.
    if (sConfig.playerBots.Enabled) {
        LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
        if (lsc != nullptr) {
            LSCChannel* chan = lsc->GetChannelByID((int32)pSystem->GetID());
            if (chan != nullptr)
                chan->AddBotChar(useCharID, useCorpID, useAllianceID, 0, useName);
        }
    }
}

void BotMgr::SpawnBotArriving(SystemManager* origin, uint32 destSystem)
{
    if (origin == nullptr || destSystem == 0)
        return;
    SpawnBot(origin, 0, "", 0, 0);
    // The bot we just created is the last one in the origin system; have it fly
    // through the gate toward destSystem (visible 12-20s warp), then cross.
    for (auto& [id, se] : origin->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (pb != nullptr && !pb->IsTraveling() && !pb->WantsToTravel()) {
            pb->SetTravelDestination(destSystem);
            pb->MarkForTravel(destSystem);
            _log(BOT__TRACE, "BotMgr: %s(%u) inbound to system %u via gate.",
                 pb->GetBotName().c_str(), pb->GetBotCharID(), destSystem);
            break;
        }
    }
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

void BotMgr::ProcessTravel()
{
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    if (sEntityList.GetSystems().empty())
        return;

    // For each loaded system with bots, with a small chance per tic, a bot heads
    // for the gate (visible warp). Once its warp timer expires (it has actually
    // reached the gate), move it to a neighbouring system.
    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr)
            continue;

        std::vector<PlayerBot*> readyToJump;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb == nullptr)
                continue;
            if (pb->WantsToTravel())
                readyToJump.push_back(pb);   // visible flight to the gate is done
            else if (MakeRandomInt(0, 299) == 0)   // ~0.33% per tic decides to leave
                pb->MarkForTravel();               // starts the visible warp
        }

        for (PlayerBot* pb : readyToJump) {
            // Cross the gate. Use the bot's requested destination if set
            // (e.g. arriving into a player's system), else pick a random one.
            uint32 destSystem = pb->GetTravelDestination();
            if (destSystem == 0)
                destSystem = GetRandomAdjacentSystem(pSystem->GetID());
            pb->ClearTravel();
            if (destSystem == 0)
                continue;   // dead-end system or no map data — stay put

            uint32 charID = pb->GetBotCharID();
            std::string name = pb->GetBotName();
            uint32 corp = pb->GetBotCorpID();
            uint32 ally = pb->GetBotAllianceID();
            _log(BOT__MESSAGE, "BotMgr: %s(%u) crossing gate to system %u (from %u).",
                 name.c_str(), charID, destSystem, pSystem->GetID());

            // Remove from the local channel and the old system.
            LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
            if (lsc != nullptr) {
                LSCChannel* chan = lsc->GetChannelByID((int32)pSystem->GetID());
                if (chan != nullptr)
                    chan->RemoveBotChar(charID);
            }
            pb->Delete();   // removes SE + item

            // Spawn in the destination system (arrives through its gate).
            SystemManager* dest = sEntityList.FindOrBootSystem(destSystem);
            if (dest != nullptr)
                SpawnBot(dest, charID, name, corp, ally);
        }
    }
}

uint32 BotMgr::GetRandomAdjacentSystem(uint32 systemID)
{
    DBQueryResult res;
    std::vector<uint32> targets;
    if (sDatabase.RunQuery(res,
        "SELECT toSolarSystemID FROM mapSolarSystemJumps WHERE fromSolarSystemID = %u", systemID))
    {
        DBResultRow row;
        while (res.GetRow(row))
            targets.push_back(row.GetUInt(0));
    }
    if (targets.empty())
        return 0;
    return targets[MakeRandomInt(0, (int64)targets.size() - 1)];
}

void BotMgr::ProcessDocking()
{
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;

    time_t now = time(nullptr);

    // 1) Undock bots whose wait is over: spawn them at the station, remove from
    //    the docked list. They'll head for the gate and leave via ProcessTravel.
    for (auto it = m_docked.begin(); it != m_docked.end(); ) {
        // Only undock into a system that is actually loaded (has players).
        if (!sEntityList.IsSystemLoaded(it->first)) { ++it; continue; }
        SystemManager* pSystem = sEntityList.FindOrBootSystem(it->first);
        if (pSystem == nullptr) { ++it; continue; }

        for (auto db = it->second.begin(); db != it->second.end(); ) {
            if (db->undockAt > 0 && db->undockAt > now) { ++db; continue; }
            _log(BOT__MESSAGE, "BotMgr: %s(%u) undocking from station in system %u.",
                 db->name.c_str(), db->charID, it->first);
            SpawnBot(pSystem, db->charID, db->name, db->corpID, db->allianceID);
            db = it->second.erase(db);
        }
        if (it->second.empty())
            it = m_docked.erase(it);
        else
            ++it;
    }

    // 2) Dock some space bots: remove their SE, keep them in local as docked.
    //    Only dock bots that are NOT mid-travel, so we don't interrupt the
    //    visible gate flight.
    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr || pSystem->PlayerCount() < 1)
            continue;
        std::vector<PlayerBot*> toDock;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb == nullptr || pb->WantsToTravel())
                continue;
            if (MakeRandomInt(0, 599) == 0)   // ~0.17% per tic decides to dock
                toDock.push_back(pb);
        }
        for (PlayerBot* pb : toDock) {
            DockedBot db;
                db.charID = pb->GetBotCharID();
                db.name = pb->GetBotName();
                db.corpID = pb->GetBotCorpID();
                db.allianceID = pb->GetBotAllianceID();
                db.undockAt = now + MakeRandomInt(60, 900);   // docked for 1-15 min
            m_docked[pSystem->GetID()].push_back(db);
            _log(BOT__MESSAGE, "BotMgr: %s(%u) docking at station in system %u.",
                 db.name.c_str(), db.charID, pSystem->GetID());
            pb->Delete();   // remove from space; stays in local channel as docked
        }
    }
}

void BotMgr::HandleLocalMessage(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message)
{
    if (!m_initalized || !sConfig.playerBots.Enabled || !sConfig.playerBots.ChatEnabled)
        return;
    if (sConfig.playerBots.DeepSeekKey.empty())
        return;

    // Find the system that owns this channel (channelID == systemID for local).
    auto& systems = sEntityList.GetSystems();
    auto it = systems.find((uint32)channelID);
    if (it == systems.end())
        return;
    SystemManager* pSystem = it->second;
    if (pSystem == nullptr)
        return;

    // Find a bot in that system (other than the sender — bots never message each
    // other's own ID here, but guard anyway).
    PlayerBot* responder = nullptr;
    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (pb != nullptr && pb->GetBotCharID() != senderCharID) {
            responder = pb;
            break;
        }
    }
    if (responder == nullptr)
        return;

    // Only react sometimes (ChatChance %) to avoid spamming on every line.
    if (MakeRandomInt(0, 99) >= sConfig.playerBots.ChatChance)
        return;

    // Throttle: at most one DeepSeek call per channel per 30s (the call blocks
    // this tick briefly; keeping it rare protects the game loop).
    time_t now = time(nullptr);
    auto last = m_lastChatReply.find(channelID);
    if (last != m_lastChatReply.end() && (now - last->second) < 30)
        return;
    m_lastChatReply[channelID] = now;

    _log(BOT__MESSAGE, "BotMgr: %s(%u) reacting to local chat from %s in system %u.",
         responder->GetBotName().c_str(), responder->GetBotCharID(), senderName.c_str(), (uint32)channelID);

    std::string prompt = senderName + " says: \"" + message + "\"";
    std::string systemHint =
        "You are a player in the MMO Eve Online in local chat. Reply as a natural, "
        "friendly EVE player. Use casual EVE slang (spaceship, isk, ratting, docking, "
        "gate, warp, fit, lowsec, nullsec). Keep it to 1-2 short sentences. English only.";

    std::string reply = BotChat::QueryDeepSeek(prompt, systemHint);
    if (reply.empty())
        return;

    // Post the reply to the system's local channel as this bot.
    LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
    if (lsc == nullptr) return;
    LSCChannel* chan = lsc->GetChannelByID(channelID);
    if (chan != nullptr)
        chan->SendBotMessage(responder->GetBotCharID(), responder->GetBotName(),
                             responder->GetBotCorpID(), reply);

    // Self-learning: this bot sent a chat line. Record it (persisted).
    if (responder->GetMemory() != nullptr) {
        responder->GetMemory()->RecordChatLine();
        responder->GetMemory()->Save();
    }
}

void BotMgr::HandleLocalReply(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message)
{
    // A real player replied in a channel where a bot recently spoke — treat it
    // as a reply to that bot (positive reinforcement for its chat tone).
    if (!m_initalized || !sConfig.playerBots.Enabled || !sConfig.playerBots.ChatEnabled)
        return;
    // Only count as a "reply" if a bot spoke here within the last 60s.
    time_t now = time(nullptr);
    auto last = m_lastChatReply.find(channelID);
    if (last == m_lastChatReply.end() || (now - last->second) > 60)
        return;

    auto& systems = sEntityList.GetSystems();
    auto it = systems.find((uint32)channelID);
    if (it == systems.end())
        return;
    SystemManager* pSystem = it->second;
    if (pSystem == nullptr)
        return;

    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (pb == nullptr || pb->GetBotCharID() == senderCharID)
            continue;
        if (pb->GetMemory() == nullptr)
            continue;
        pb->GetMemory()->RecordChatReply();
        pb->GetMemory()->Save();
        _log(BOT__TRACE, "BotMgr: %s(%u) chat line got a reply from %s.",
             pb->GetBotName().c_str(), pb->GetBotCharID(), senderName.c_str());
        return;
    }
}
