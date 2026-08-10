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
#include "account/AccountDB.h"
#include "account/Account.h"
#include <cctype>

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

    // Experienced leader hunters occasionally found their own corporations.
    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr)
            continue;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr) {
                MaybeFoundCorp(pb);
                MaybeFormAlliance(pb);
                ProcessEconomy(pb);
            }
        }
    }
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

    time_t now = time(nullptr);

    // Target per system, re-rolled every ~5 min so the local count drifts
    // up/down (a live server's population breathes, not a fixed number).
    auto tIt = m_systemTarget.find(pSystem->GetID());
    if (tIt == m_systemTarget.end()) {
        uint32 t0 = (uint32)(sConfig.playerBots.MaxPerSystem * (0.6f + MakeRandomFloat() * 0.4f));
        m_systemTarget[pSystem->GetID()] = t0;
    }
    // Occasionally re-roll: previous ± random drift, clamped 40%..100% of cap.
    {
        static std::map<uint32, time_t> s_lastTargetRoll;
        auto lr = s_lastTargetRoll.find(pSystem->GetID());
        if (lr == s_lastTargetRoll.end() || (now - lr->second) >= 300) {
            s_lastTargetRoll[pSystem->GetID()] = now;
            uint32 base = m_systemTarget[pSystem->GetID()];
            int nt = (int)base + (int)(base * (MakeRandomFloat() * 0.4f - 0.2f));
            uint32 minT = (uint32)(sConfig.playerBots.MaxPerSystem * 0.4f);
            if (nt < (int)minT) nt = (int)minT;
            if (nt > (int)sConfig.playerBots.MaxPerSystem) nt = (int)sConfig.playerBots.MaxPerSystem;
            m_systemTarget[pSystem->GetID()] = (uint32)nt;
        }
    }
    uint32 target = m_systemTarget[pSystem->GetID()];
    if (botCount >= target)
        return;

    // Gradual fill: spawn AT MOST one bot per ~15s per system, so the population
    // trickles in over minutes (like a live server) instead of all at once.
    auto last = m_lastPopulate.find(pSystem->GetID());
    if (last != m_lastPopulate.end() && (now - last->second) < 15)
        return;
    m_lastPopulate[pSystem->GetID()] = now;

    // Variety: some bots are already in the system (docked or in space), others
    // arrive through a gate, others leave. Decide per spawn.
    uint32 spawnMode = MakeRandomInt(0, 9);
    if (spawnMode < 3) {
        // Already here — spawn directly at a gate / station in this system
        // (they "live" here), no inbound flight.
        SpawnBot(pSystem, 0, "", 0, 0);
        // Some of these are just passing through — leave shortly.
        if (MakeRandomInt(0, 99) < 25) {
            for (auto& [id, se] : pSystem->GetEntities()) {
                if (se == nullptr || se->GetNPCSE() == nullptr)
                    continue;
                PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
                if (pb != nullptr && !pb->IsTraveling() && !pb->WantsToTravel()) {
                    pb->MarkForTravel();
                    break;
                }
            }
        }
    } else {
        // Inbound through a gate from a neighbouring system (visible warp).
        uint32 origin = GetRandomAdjacentSystem(pSystem->GetID());
        if (origin == 0) {
            SpawnBot(pSystem, 0, "", 0, 0);
            return;
        }
        SystemManager* originSys = sEntityList.FindOrBootSystem(origin);
        if (originSys == nullptr)
            return;
        SpawnBotArriving(originSys, pSystem->GetID());
    }
}

uint32 BotMgr::PickCorp(uint32& allianceID, bool requireAlliance /*false*/)
{
    // Pick a corp that actually exists in this server's crpCorporation table
    // (179 seeded NPC corps). Killmail corp ids (live EVE) don't exist locally,
    // so a bot in such a corp breaks the info window (no HQ found). Weight by
    // existing member count so the biggest corp takes the largest share.
    // PvP war corps (requireAlliance) only get corps that are in an alliance.
    std::string corpQuery = std::string(
        "SELECT c.corporationID, c.allianceID, COUNT(ch.characterID) AS members"
        " FROM crpCorporation c"
        " LEFT JOIN chrCharacters ch ON ch.corporationID = c.corporationID"
        " WHERE c.corporationID >= 1000000")      // skip 0/placeholder rows
        + (requireAlliance ? " AND c.allianceID > 0" : "")
        + std::string(" GROUP BY c.corporationID")
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
    // Corp: ALWAYS from the local crpCorporation table. Killmail corp ids are
    // from live EVE and don't exist in this server's DB — a bot in such a corp
    // breaks the info window ('No HQ found for corporation'). Bots are real
    // members of the chosen corp, so distribution self-organizes.
    useCorpID = PickCorp(useAllianceID, prof == PlayerBot::BotProfession::Hunter);
    if (useCorpID == 0) {
        _log(BOT__ERROR, "BotMgr: no corporation available for bot, skipping spawn.");
        return;
    }

    // Persist the bot as a REAL character (chrCharacters + portrait + skills +
    // history) so its legend and progress survive restarts. The character id is
    // allocated normally (sequential free id); CreateBotCharacter de-dupes by name.
    {
        uint8 skillTier = sConfig.playerBots.MinSkillLevel +
            MakeRandomInt(0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
        useCharID = CharacterDB::CreateBotCharacter(useName, useCorpID, useAllianceID, skillTier);
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

    // Real players almost always rename their ship to something arbitrary
    // (a word, a name, a code). Give the bot's hull a random ship name too,
    // NOT the pilot's name — a pilot named after their ship is a tell.
    std::string shipName = MakeRandomShipName();
    ItemData idata(hullType, useCorpID, pSystem->GetID(), flagNone, shipName.c_str(), pos);
    InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
    if (iRef.get() == nullptr) {
        _log(BOT__ERROR, "BotMgr: failed to spawn ship hull %u for bot.", hullType);
        return;
    }

    FactionData data = FactionData();
    data.corporationID = useCorpID;
    data.ownerID = useCharID;   // pilot owns the ship (client locks + shows owner)
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

std::string BotMgr::MakeRandomShipName()
{
    // Players name hulls all sorts of ways: made-up words, callsigns, quotes,
    // memes, even simple ASCII/box-drawing art. Mirror that variety.
    static const char* pre[] = { "Re", "Ve", "Ka", "Ni", "Za", "Xo", "Ma", "Ta", "Ru", "Di", "Fo", "Ly" };
    static const char* mid[] = { "li", "ra", "no", "va", "su", "ro", "ma", "ke", "tu", "go", "pa", "ze" };
    static const char* suf[] = { "ra", "tor", "rix", "lon", "gus", "nar", "yen", "dax", "mir", "kus", "vel", "tar" };
    static const char* nbr[] = { "", "-1", "-7", "-X", " II", " V", "9", "13", "-A" };

    // Meme / phrase / culture names (EVE inside jokes, movie, game, song refs).
    static const char* phrases[] = {
        "Trust no one", "Fly safe", "Loot fairy", "This is fine", "Almost there",
        "Space trucker", "Not a ganker", "Show me the isk", "Miner tears",
        "Oops", "Free ride", "One more jump", "Pod me please", "No cap needed",
        "RIP my wallet", "Eject soon", "Titan of industry", "Capsuleer's bane",
        "The Answer", "So it goes", "Never tell me the odds", "I aim to misbehave",
        "Do you even warp", "Belt is lava", "Scanned & panned", "Lowsec vacuum",
    };
    // Simple box-drawing / ASCII art names (client renders them fine).
    static const char* art[] = {
        "<====>", "[--==--]", ">==>", "{::}", "[-]~>", "<=<>=<", "*==*", ">o=>", "}{}{}", "|==|",
    };

    switch (MakeRandomInt(0, 3)) {
        case 0: {   // made-up word
            std::string n = std::string(pre[MakeRandomInt(0, 11)]) + mid[MakeRandomInt(0, 11)] + suf[MakeRandomInt(0, 11)];
            n += nbr[MakeRandomInt(0, 8)];
            return n;
        }
        case 1: {   // capitalised made-up word
            std::string n = std::string(pre[MakeRandomInt(0, 11)]) + mid[MakeRandomInt(0, 11)] + suf[MakeRandomInt(0, 11)];
            n[0] = (char)toupper(n[0]);
            return n;
        }
        case 2:     // phrase / meme
            return phrases[MakeRandomInt(0, 25)];
        default:    // box-drawing / ASCII art
            return art[MakeRandomInt(0, 9)];
    }
}

std::string BotMgr::MakeCorpName()
{
    static const char* pre[] = { "Serpent", "Iron", "Void", "Solar", "Night", "Star", "Ghost", "Red", "Black", "Golden" };
    static const char* suf[] = { " Industries", " Holdings", " Trading", " Dynamics", " Logistics", " Syndicate", " Group", " Alliance Services" };
    std::string n = pre[MakeRandomInt(0, 9)];
    n += suf[MakeRandomInt(0, 7)];
    return n;
}

std::string BotMgr::MakeTicker()
{
    static const char* base[] = { "SRP", "IRN", "VOD", "SOL", "NGT", "STR", "GHT", "RDB", "BLK", "GLD" };
    return std::string(base[MakeRandomInt(0, 9)]) + std::string(base[MakeRandomInt(0, 9)]);
}

int64 BotMgr::MakeCorpLogo(uint32 seed, uint8 slot)
{
    // Deterministic pseudo-random from the seed (founder char id). Slot mapping:
    // 0 -> graphicID (1447..1627, matches the seeded corp logo range)
    // 1-3 -> colors as 0xRRGGBB ints
    // 4-6 -> shapes (0..23)
    uint32 h = seed * 2654435761u + slot * 40503u;   // golden-ratio hash
    h = (h ^ (h >> 16)) * 2246822519u;
    switch (slot) {
        case 0:  return 1447 + (h % 181);
        case 1:  return (int64)(h & 0xFFFFFF);           // color 1
        case 2:  return (int64)((h >> 8) & 0xFFFFFF);    // color 2
        case 3:  return (int64)((h >> 16) & 0xFFFFFF);   // color 3
        default: return h % 24;                          // shapes 0..23
    }
}

void BotMgr::MaybeFoundCorp(PlayerBot* bot)
{
    // A bot-founder is a practised leader (hunter profession, high skill tier)
    // with enough experience. Starts in an NPC corp, then founds its own.
    if (bot == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (bot->GetProfession() != PlayerBot::BotProfession::Hunter)
        return;   // only leader-type pilots found corps
    if (bot->GetBotSkillLevel() < 5)
        return;
    float practice = bot->GetMemory() ? bot->GetMemory()->GetActivitySkill() : 0.0f;
    if (MakeRandomInt(0, 999) >= (int)(20 + practice * 80))
        return;   // rare, and more likely with practice

    uint32 charID = bot->GetBotCharID();
    uint32 oldCorp = bot->GetBotCorpID();

    // Build a new corp owned by this bot (CEO = founder). Logo derived from the
    // new corp id (deterministic): a real-EVE-style logo = graphicID (1447-1627)
    // + 3 colours + 3 shapes, so every bot corp looks like a real player corp.
    std::string cName = MakeCorpName();
    std::string ticker = MakeTicker();
    DBerror err;
    uint32 corpID = 0;
    if (!sDatabase.RunQueryLID(err, corpID,
        "INSERT INTO crpCorporation"
        "  (corporationName, description, tickerName, url, taxRate, corporationType, hasPlayerPersonnelManager,"
        "   creatorID, ceoID, stationID, raceID, shares, memberLimit, allowedMemberRaceIDs,"
        "   graphicID, color1, color2, color3, shape1, shape2, shape3, isRecruiting, allianceMemberStartDate)"
        " VALUES"
        "  ('%s', 'Bot-founded', '%s', '', 0.1, 2, 1,"
        "   %u, %u, 60000004, 1, 1000, 100, 1,"
        "   %u, %u, %u, %u, %u, %u, %u, 1, 0)",
        cName.c_str(), ticker.c_str(), charID, charID,
        MakeCorpLogo(charID, 0), MakeCorpLogo(charID, 1), MakeCorpLogo(charID, 2), MakeCorpLogo(charID, 3),
        MakeCorpLogo(charID, 4), MakeCorpLogo(charID, 5), MakeCorpLogo(charID, 6)))
    {
        codelog(DATABASE__ERROR, "MaybeFoundCorp: corp insert failed: %s", err.c_str());
        return;
    }

    // Default corp wallet/autopay/share rows (mirror AddCorporation).
    sDatabase.RunQuery(err, "INSERT INTO crpWalletDivisons (corporationID) VALUES (%u)", corpID);
    sDatabase.RunQuery(err, "INSERT INTO crpAutoPay (corporationID) VALUES (%u)", corpID);
    sDatabase.RunQuery(err, "INSERT INTO crpShares (corporationID, shareholderID, shares, shareholderCorporationID)"
                            " VALUES (%u, %u, 1000, %u)", corpID, corpID, corpID);
    sDatabase.RunQuery(err, "INSERT INTO eveStaticOwners (ownerID, ownerName, typeID) VALUES (%u, '%s', 2)",
                       corpID, cName.c_str());

    // Transfer the founder to the new corp (employment history recorded).
    CharacterDB::AddEmployment(charID, corpID, oldCorp);

    _log(BOT__MESSAGE, "BotMgr: %s(%u) founded corp %s [%s] (%u), left corp %u.",
         bot->GetBotName().c_str(), charID, cName.c_str(), ticker.c_str(), corpID, oldCorp);

    // The founder recruits a few like-minded bots (same NPC corp they just left)
    // into the new corporation — the corp grows from one pilot to a small group.
    uint32 recruited = 0;
    if (bot->SystemMgr() != nullptr) {
        for (auto& [rid, rse] : bot->SystemMgr()->GetEntities()) {
            if (recruited >= 4)
                break;
            if (rse == nullptr || rse->GetNPCSE() == nullptr)
                continue;
            PlayerBot* rbot = dynamic_cast<PlayerBot*>(rse->GetNPCSE());
            if (rbot == nullptr || rbot == bot)
                continue;
            if (rbot->GetBotCorpID() != oldCorp)
                continue;   // only pull from the corp the founder left
            CharacterDB::AddEmployment(rbot->GetBotCharID(), corpID, oldCorp);
            _log(BOT__TRACE, "BotMgr: %s(%u) recruited into %s.", rbot->GetBotName().c_str(), rbot->GetBotCharID(), cName.c_str());
            ++recruited;
        }
    }

    // The corp keeps growing; a practised founder later unites it with other
    // bot-founded corps into an alliance (MaybeFormAlliance).
}

std::string BotMgr::MakeAllianceName()
{
    static const char* pre[] = { "Void", "Iron", "Solar", "Night", "Star", "Ghost", "Red", "Black", "Golden", "Crimson" };
    static const char* suf[] = { " Alliance", " Coalition", " Federation", " Pact", " Bloc", " Union", " Concord", " Compact" };
    std::string n = pre[MakeRandomInt(0, 9)];
    n += suf[MakeRandomInt(0, 7)];
    return n;
}

void BotMgr::MaybeFormAlliance(PlayerBot* bot)
{
    // A practised founder (hunter) unites several bot-founded corps into an
    // alliance when its own corp is big enough. Grouping is by profession or
    // location: find other bot-founded corps in the same region, or same
    // profession, with enough total members.
    if (bot == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (bot->GetProfession() != PlayerBot::BotProfession::Hunter)
        return;
    if (bot->GetBotSkillLevel() < 5)
        return;
    float practice = bot->GetMemory() ? bot->GetMemory()->GetActivitySkill() : 0.0f;
    if (MakeRandomInt(0, 999) >= (int)(10 + practice * 60))
        return;

    uint32 myCorp = bot->GetBotCorpID();

    // Find bot-founded corps (CEO is a bot character, not an NPC corp owner).
    // Group by location/profession: other bot corps not yet in an alliance.
    DBQueryResult res;
    std::vector<uint32> memberCorps;
    memberCorps.push_back(myCorp);
    if (sDatabase.RunQuery(res,
        "SELECT c.corporationID"
        " FROM crpCorporation c"
        " WHERE c.ceoID >= 90000000"      // bot-founded corps (CEO is a bot char id)
        "   AND c.corporationID <> %u"
        "   AND c.allianceID = 0"          // not already in an alliance
        " LIMIT 6", myCorp))
    {
        DBResultRow row;
        while (res.GetRow(row))
            memberCorps.push_back(row.GetUInt(0));
    }

    // Need at least 2 bot corps to justify an alliance.
    if (memberCorps.size() < 2)
        return;

    // Check total membership across those corps.
    uint32 totalMembers = 0;
    for (uint32 cid : memberCorps) {
        DBQueryResult mr;
        if (sDatabase.RunQuery(mr, "SELECT COUNT(*) FROM chrCharacters WHERE corporationID = %u", cid)) {
            DBResultRow rrow;
            if (mr.GetRow(rrow))
                totalMembers += rrow.GetUInt(0);
        }
    }
    if (totalMembers < 6)
        return;   // alliance needs critical mass

    // Create the alliance (executor = founder's corp).
    std::string aName = MakeAllianceName();
    std::string aShort = MakeTicker();
    DBerror err;
    uint32 allyID = 0;
    if (!sDatabase.RunQueryLID(err, allyID,
        "INSERT INTO alnAlliance (allianceName, shortName, description, executorCorpID, creatorCorpID, creatorCharID, startDate, memberCount, url)"
        " VALUES ('%s', '%s', 'Bot-founded', %u, %u, %u, %f, %u, '')",
        aName.c_str(), aShort.c_str(), myCorp, myCorp, bot->GetBotCharID(), GetFileTimeNow(), (uint32)memberCorps.size()))
    {
        codelog(DATABASE__ERROR, "MaybeFormAlliance: alliance insert failed: %s", err.c_str());
        return;
    }
    sDatabase.RunQuery(err, "INSERT INTO eveStaticOwners (ownerID, ownerName, typeID) VALUES (%u, '%s', 16159)", allyID, aName.c_str());

    // Set the alliance on all member corps.
    for (uint32 cid : memberCorps) {
        sDatabase.RunQuery(err, "UPDATE crpCorporation SET allianceID = %u, allianceMemberStartDate = %f WHERE corporationID = %u",
                           allyID, GetFileTimeNow(), cid);
    }

    _log(BOT__MESSAGE, "BotMgr: %s(%u) formed alliance %s [%s] (%u) from %u corps.",
         bot->GetBotName().c_str(), bot->GetBotCharID(), aName.c_str(), aShort.c_str(), allyID, (uint32)memberCorps.size());
}

void BotMgr::GetDockedAtStation(uint32 stationID, std::vector<GuestInfo>& out) const
{
    // Docked bots are stored per-system in m_docked. Map the station back to its
    // system and return the bots docked there (for the station pilots window).
    if (m_docked.empty())
        return;
    // Resolve the station's solar system once.
    uint32 sysID = 0;
    DBQueryResult res;
    if (sDatabase.RunQuery(res, "SELECT solarSystemID FROM staStations WHERE stationID = %u", stationID)) {
        DBResultRow row;
        if (res.GetRow(row))
            sysID = row.GetUInt(0);
    }
    if (sysID == 0)
        return;
    auto it = m_docked.find(sysID);
    if (it == m_docked.end())
        return;
    for (const auto& db : it->second) {
        GuestInfo g;
            g.charID = db.charID;
            g.corpID = db.corpID;
            g.allianceID = db.allianceID;
            g.warFactionID = 0;
        out.push_back(g);
    }
}

void BotMgr::ProcessEconomy(PlayerBot* bot)
{
    // Bots take part in the EVE economy with real ISK:
    //  - traders place sell orders in their own name,
    //  - everyone pays corp tax into their corp wallet.
    if (bot == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (bot->GetProfession() == PlayerBot::BotProfession::Trader)
        PlaceBotOrder(bot);
    if (MakeRandomInt(0, 999) < 30)
        PayCorpTax(bot);
}

void BotMgr::PayCorpTax(PlayerBot* bot)
{
    // Corp tax: a fraction of the bot's income flows into the corp wallet.
    uint32 charID = bot->GetBotCharID();
    uint32 corpID = bot->GetBotCorpID();
    if (corpID == 0)
        return;

    DBQueryResult res;
    double balance = 0;
    if (sDatabase.RunQuery(res, "SELECT balance FROM chrCharacters WHERE characterID = %u", charID)) {
        DBResultRow row;
        if (res.GetRow(row))
            balance = row.GetDouble(0);
    }
    if (balance < 1000000)
        return;   // keep a minimum on the pilot

    double tax = balance * 0.02;   // 2% corp tax
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET balance = balance - %f WHERE characterID = %u", tax, charID);
    // credit the corp wallet (cash division). Corp may not have a wallet row yet
    // (NPC corps don't), so upsert.
    sDatabase.RunQuery(err,
        "INSERT INTO crpWalletDivisons (corporationID, balance1) VALUES (%u, %f)"
        " ON DUPLICATE KEY UPDATE balance1 = balance1 + %f",
        corpID, tax, tax);
    _log(BOT__TRACE, "BotMgr: %s(%u) paid %.0f ISK corp tax to corp %u.",
         bot->GetBotName().c_str(), charID, tax, corpID);
}

void BotMgr::PlaceBotOrder(PlayerBot* bot)
{
    // Trader bots sell goods on the market in their own name. Orders are real
    // mktOrders rows (visible to players); proceeds are credited to the bot's
    // wallet via direct balance update when the order fills is handled by the
    // market proxy — here we only create the sell listing.
    uint32 charID = bot->GetBotCharID();
    uint32 sysID = bot->SystemMgr() ? bot->SystemMgr()->GetID() : 0;
    if (sysID == 0)
        return;

    // A small pool of commonly-traded commodities.
    static const uint32 goods[] = { 34, 38, 39, 40, 3775, 2488, 2048, 2676, 1229, 1230 };   // ammo, minerals, drone
    uint32 typeID = goods[MakeRandomInt(0, 9)];

    DBQueryResult res;
    uint32 stationID = 0;
    if (sDatabase.RunQuery(res,
        "SELECT stationID FROM staStations WHERE solarSystemID = %u LIMIT 1", sysID)) {
        DBResultRow row;
        if (res.GetRow(row))
            stationID = row.GetUInt(0);
    }
    if (stationID == 0)
        return;

    double price = 100.0 + (MakeRandomInt(0, 90000) / 100.0);
    uint32 qty = MakeRandomInt(10, 500);

    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO mktOrders"
        "  (typeID, ownerID, regionID, stationID, solarSystemID, orderRange, bid, price,"
        "   escrow, minVolume, volEntered, volRemaining, issued, duration, isCorp, accountKey, memberID)"
        " VALUES"
        "  (%u, %u, (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u, 32767, 0, %f,"
        "   0, 1, %u, %u, %f, 90, 0, 1000, %u)",
        typeID, charID, sysID, stationID, sysID, price, qty, qty, GetFileTimeNow(), charID);
    _log(BOT__TRACE, "BotMgr: %s(%u) placed sell order %ux type %u @ %.2f ISK in %u.",
         bot->GetBotName().c_str(), charID, qty, typeID, price, sysID);
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

    // 2) Dock space bots: remove their SE, keep them in local as docked.
    //    Only dock bots that are NOT mid-travel, so we don't interrupt the
    //    visible gate flight. Bots that WANT to dock (end of mining run, traders
    //    at the market) dock immediately; others dock occasionally.
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
            if (pb->WantsDock())
                toDock.push_back(pb);                       // profession wants the station now
            else if (MakeRandomInt(0, 599) == 0)            // occasional dock
                toDock.push_back(pb);
        }
        for (PlayerBot* pb : toDock) {
            DockedBot db;
                db.charID = pb->GetBotCharID();
                db.name = pb->GetBotName();
                db.corpID = pb->GetBotCorpID();
                db.allianceID = pb->GetBotAllianceID();
                // Traders/market guys sit longer; miners refine/sell quickly then head out.
                db.undockAt = now + (pb->GetProfession() == PlayerBot::BotProfession::Trader
                                     ? MakeRandomInt(300, 1800)    // 5-30 min at market
                                     : MakeRandomInt(30, 300));    // 0.5-5 min for everyone else
            m_docked[pSystem->GetID()].push_back(db);
            _log(BOT__MESSAGE, "BotMgr: %s(%u) docking at station in system %u.",
                 db.name.c_str(), db.charID, pSystem->GetID());
            pb->ClearDockRequest();
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
