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
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <vector>

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

    // Docked traders work the market from their station.
    ProcessDockedEconomy();

    // Bots occasionally chatter among themselves in local (rare).
    ProcessBotSmalltalk();

    // Drain queued bot chat replies (one per tic) — drives bot<-bot conversations
    // without recursing the stack.
    ProcessBotReplies();

    // Courier bots pick up unaccepted player courier contracts.
    ProcessPlayerContracts();

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
    DBQueryResult res;
    std::vector<std::pair<uint32,uint32>> corps;   // corpID, allianceID
    std::vector<uint32> weights;                    // members+1 per corp
    std::string corpQuery = std::string(
        "SELECT c.corporationID, c.allianceID, COUNT(ch.characterID) AS members"
        " FROM crpCorporation c"
        " LEFT JOIN chrCharacters ch ON ch.corporationID = c.corporationID"
        " WHERE c.corporationID >= 1000000")      // skip 0/placeholder rows
        + " AND NOT EXISTS ("                     // never join a corp a REAL player is in
            " SELECT 1 FROM chrCharacters realc"
            " WHERE realc.corporationID = c.corporationID AND realc.accountID != 0)"
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

void BotMgr::SpawnBot(SystemManager* pSystem, uint32 charID, const std::string& name, uint32 corpID, uint32 allianceID, bool arrivedViaGate /*false*/)
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

    // Random/unspecified spawn (charID==0): prefer REUSING an already-created bot
    // instead of rolling a fresh random legend every time. Bots persist as real
    // characters (chrCharacters + botMemory), so a returning pilot should be the
    // SAME pilot it always was — same face, same name, same learned history —
    // not a random new legend each respawn. Only roll a fresh legend when no
    // established bot is available.
    if (useCharID == 0 && useName.empty()) {
        DBQueryResult bres;
        if (sDatabase.RunQuery(bres,
            "SELECT c.characterName, c.corporationID, cc.allianceID"
            " FROM chrCharacters c"
            " JOIN botMemory b ON b.charID = c.characterID"
            " LEFT JOIN crpCorporation cc ON cc.corporationID = c.corporationID"
            " WHERE c.characterName != ''"
            " ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow brow;
            if (bres.GetRow(brow)) {
                useName = brow.GetText(0);
                useCorpID = brow.GetUInt(1);
                useAllianceID = brow.GetUInt(2);
                _log(BOT__TRACE, "BotMgr: reusing established bot '%s' (corp %u, ally %u).",
                     useName.c_str(), useCorpID, useAllianceID);
            }
        }
    }

    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT character_id, character_name, corporation_id, alliance_id,"
            "       ship_type_id, fitted_item_ids"
            " FROM botKillmailLegends"
            " WHERE ship_type_id > 0 AND ship_type_id != 670"   // no capsule legends (pod kills)
            "   AND character_name != ''"
            " ORDER BY RAND() LIMIT 1"))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                useCharID = row.GetUInt(0);   // real EVE killmail charID — kept for the portrait link
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

    // Corp comes from the bot's STARTING SCHOOL — exactly like a real newbie who
    // picks a faction at character creation. CreateBotCharacter picks a bloodline
    // (race) then the school that race graduates from and returns the corp that
    // runs it (Imperial Academy, State War Academy, ...). A bot is thus a real
    // member of its faction's starter corp — not "Rogue Drone" or "Serpentis".
    // (Hunter PvP war corps still need an alliance to claim nullsec sovereignty;
    // that's handled later by MaybeFormAlliance once the bot proves itself.)

    // Persist the bot as a REAL character (chrCharacters + portrait + skills +
    // history) so its legend and progress survive restarts. The character id is
    // allocated normally (sequential free id); CreateBotCharacter de-dupes by name.
    uint32 killmailCharID = useCharID;   // real EVE id the legend came from (for portraits)
    uint8 skillTier = sConfig.playerBots.MinSkillLevel +
        MakeRandomInt(0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
    uint8 botSchoolID = 0;

    // Pick a legend whose pilot is NOT already flying in this system. CreateBotCharacter
    // de-dupes by name (returns the existing charID), so picking a legend whose name is
    // already spawned here would create a "clone" of that pilot. Retry up to N legends
    // until we find one that isn't already present in this system.
    bool botAlreadyHere = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        useCharID = CharacterDB::CreateBotCharacter(useName, useAllianceID, skillTier, useCorpID, botSchoolID);
        if (useCharID == 0) {
            _log(BOT__ERROR, "BotMgr: failed to create persisted bot character '%s'.", useName.c_str());
            return;
        }
        botAlreadyHere = false;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* existing = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (existing != nullptr && existing->GetBotCharID() == useCharID) {
                botAlreadyHere = true;
                break;
            }
        }
        if (!botAlreadyHere)
            break;
        // This pilot is already in the system — pick a fresh legend and retry.
        _log(BOT__TRACE, "BotMgr: %s(%u) already in system %u — retrying with another legend.",
             useName.c_str(), useCharID, pSystem->GetID());
        if (attempt == 7)
            break;   // give up after retries; caller will skip (no duplicate SE)
        DBQueryResult lres;
        if (!sDatabase.RunQuery(lres,
            "SELECT character_id, character_name, corporation_id, alliance_id,"
            "       ship_type_id, fitted_item_ids"
            " FROM botKillmailLegends"
            " WHERE ship_type_id > 0 AND ship_type_id != 670"   // no capsule legends (pod kills)
            "   AND character_name != ''"
            " ORDER BY RAND() LIMIT 1"))
            break;
        DBResultRow lrow;
        if (!lres.GetRow(lrow))
            break;
        useName = lrow.GetText(1);
        useCorpID = lrow.GetUInt(2);
        useAllianceID = lrow.GetUInt(3);
        useShipType = lrow.GetUInt(4);
        const char* fit = lrow.GetText(5);
        if (fit != nullptr) useFit = fit;
        // NOTE: do NOT set useCharID from the legend here — CreateBotCharacter
        // allocates the real charID on the next loop iteration.
    }
    if (botAlreadyHere) {
        _log(BOT__TRACE, "BotMgr: all candidate legends already in system %u — skipping spawn.",
             pSystem->GetID());
        return;
    }
    // Remember the EVE portrait source so fetch_bot_portraits.py can grab it —
    // AND download it now (async) so the client sees a face immediately.
    if (killmailCharID != 0 && killmailCharID != useCharID) {
        DBerror perr;
        sDatabase.RunQuery(perr,
            "INSERT IGNORE INTO botPortraits (serverCharID, eveCharID) VALUES (%u, %u)",
            useCharID, killmailCharID);
        FetchPortraitAsync(useCharID, killmailCharID);
    }

    // Profession: keep the bot's saved job across respawns (a miner stays a miner —
    // it's been learning it). Only brand-new pilots roll a fresh one.
    PlayerBot::BotProfession prof = PlayerBot::BotProfession::Miner;
    {
        DBQueryResult pres;
        if (sDatabase.RunQuery(pres,
            "SELECT profession FROM botMemory WHERE charID = %u AND profession != 255",
            useCharID))
        {
            DBResultRow prow;
            if (pres.GetRow(prow)) {
                prof = (PlayerBot::BotProfession)prow.GetUInt(0);
            } else {
                // New pilot — roll a profession and persist it for future respawns.
                float p = MakeRandomFloat();
                if (p < 0.10f)
                    prof = PlayerBot::BotProfession::Hunter;       // PvP pirates / war corps / guards
                else if (p < 0.25f)
                    prof = PlayerBot::BotProfession::RatHunter;    // peaceful PvE (red crosses only)
                else if (p < 0.50f)
                    prof = PlayerBot::BotProfession::Miner;        // miners (co-op with guards)
                else if (p < 0.60f)
                    prof = PlayerBot::BotProfession::Trader;       // market / station traders
                else if (p < 0.80f)
                    prof = PlayerBot::BotProfession::Courier;      // couriers: haul to/from hub
                else if (p < 0.90f)
                    prof = PlayerBot::BotProfession::Hacker;       // data/relic sites
                else
                    prof = PlayerBot::BotProfession::Explorer;     // probes / wormholes
                DBerror perr;
                sDatabase.RunQuery(perr,
                    "INSERT INTO botMemory (charID, profession, lastUpdate)"
                    " VALUES (%u, %u, NOW())"
                    " ON DUPLICATE KEY UPDATE profession = VALUES(profession), lastUpdate = NOW()",
                    useCharID, (uint8)prof);
            }
        }
    }

    // The bio is written exactly once per pilot, right after its profession is
    // rolled on the very first spawn, so it stays stable across respawns (a
    // player's bio doesn't change every time they log in). Profession-flavoured
    // text + ASCII art makes charbots read like real pilots, not a clone farm.
    CharacterDB::UpdateBotBio(useCharID, (uint8)prof);

    // Ship hull. For PEACEFUL professions the hull is always profession-fit: a
    // miner works the belt on a barge/mining frigate, a trader/courier hauls in a
    // freighter, a hacker probes in a scan frigate. A real pilot swaps ships at a
    // station (dock -> undock -> new hull), which the bot does on every undock.
    // Hunters/RatHunters fly a real killmail legend hull (or a combat cruiser/BC
    // if the legend ship doesn't exist in Crucible-era data).
    uint32 hullType = useShipType;
    {
        static const uint32 minerHulls[]  = { 17476, 17478, 17480, 582, 592, 599 };   // Covetor/Retriever/Procurer + mining frigates
        static const uint32 haulerHulls[] = { 648, 650, 651, 653, 1944 };             // Badger/Iteron/Hoarder/Wreathe/Bestower
        static const uint32 scanHulls[]   = { 605, 607, 586, 590 };                   // Heron/Imicus/Probe/Inquisitor
        static const uint32 combatHulls[] = { 621, 633, 626, 613, 609, 597, 606, 601 }; // assorted T1 cruisers/BC

        const uint32* pick = nullptr;
        uint32 pickCount = 0;
        bool forceProfessionHull = false;
        switch (prof) {
            case PlayerBot::BotProfession::Miner:
                pick = minerHulls; pickCount = sizeof(minerHulls)/sizeof(minerHulls[0]); forceProfessionHull = true;
                break;
            case PlayerBot::BotProfession::Trader:
            case PlayerBot::BotProfession::Courier:
                pick = haulerHulls; pickCount = sizeof(haulerHulls)/sizeof(haulerHulls[0]); forceProfessionHull = true;
                break;
            case PlayerBot::BotProfession::Hacker:
            case PlayerBot::BotProfession::Explorer:
                pick = scanHulls; pickCount = sizeof(scanHulls)/sizeof(scanHulls[0]); forceProfessionHull = true;
                break;
            default:   // Hunter / RatHunter — combat
                pick = combatHulls; pickCount = sizeof(combatHulls)/sizeof(combatHulls[0]);
                break;
        }

        if (forceProfessionHull) {
            hullType = pick[MakeRandomInt(0, (int32)pickCount - 1)];
        } else {
            // Combat hull from the killmail legend (real EVE hull). Killmail hull
            // types are from modern EVE — some don't exist in the server's
            // (Crucible-era) invTypes, or are pods/shuttles/deployables/#System,
            // or are NON-COMBAT ships (mining barges, freighters, haulers) that a
            // pirate/hunter would never fly. Validate it's a real combat hull and
            // fall back to a combat cruiser/BC if not.
            Inv::TypeData tdata = Inv::TypeData();
            sDataMgr.GetType((uint16)hullType, tdata);
            bool valid = (hullType != 0) && (tdata.id == hullType)
                         && (tdata.groupID != 0)      // '#System' placeholder
                         && (tdata.groupID != 29)     // Capsule
                         && (tdata.groupID != 31)     // Shuttle
                         && (tdata.groupID != 361);   // Mobile Warp Disruptor & co
            if (valid) {
                switch (tdata.groupID) {
                    case EVEDB::invGroups::Frigate:
                    case EVEDB::invGroups::Rookieship:
                    case EVEDB::invGroups::Destroyer:
                    case EVEDB::invGroups::Cruiser:
                    case EVEDB::invGroups::Battlecruiser:
                    case EVEDB::invGroups::Battleship:
                    case EVEDB::invGroups::AssaultShip:
                    case EVEDB::invGroups::HeavyAssaultShip:
                    case EVEDB::invGroups::Interceptor:
                    case EVEDB::invGroups::Interdictor:
                    case EVEDB::invGroups::CombatRecon:
                    case EVEDB::invGroups::Logistics:
                    case EVEDB::invGroups::CovertOps:
                    case EVEDB::invGroups::BlackOps:
                    case EVEDB::invGroups::Marauder:
                    case EVEDB::invGroups::EliteBattleship:
                        break;  // combat hull — keep it
                    default:
                        valid = false;   // barge/freighter/hauler/other — not a combat hull
                        break;
                }
            }
            if (!valid)
                hullType = pick[MakeRandomInt(0, (int32)pickCount - 1)];
        }
    }

    _log(BOT__MESSAGE, "BotMgr: spawning simulated player '%s' (char %u, corp %u, ship %u, fit %zu items) in system %u",
         useName.c_str(), useCharID, useCorpID, hullType, useFit.size(), pSystem->GetID());

    // Spawn near a gate — the bot "arrived through the gate from the neighbouring
    // system", matching a real pilot's travel story. Do NOT put it at the gate's
    // centre: gates have huge collision spheres (14-19km) and a ship inside one
    // gets snapped out every tick (visible micro-teleports / "repulsion"). Place
    // it just outside the gate's radius, offset toward the gate so it looks like
    // it warped in beside the gate.
    GPoint pos;
    bool posSet = false;
    uint32 arriveGateID = 0;   // the gate this bot "came through" — for the jump-in animation
    for (auto& [id, se] : pSystem->GetStaticEntities()) {
        if (se != nullptr && se->GetGateSE() != nullptr) {
            GPoint gatePos = se->GetPosition();
            double gateR = se->GetRadius() > 500.0 ? se->GetRadius() : 3000.0;
            // Random offset in the plane, 2-5 km past the gate's surface.
            double ang = MakeRandomFloat() * 6.2831853;
            GPoint offset(cos(ang) * (gateR + MakeRandomFloat() * 3000.0 + 2000.0),
                          sin(ang) * (gateR + MakeRandomFloat() * 3000.0 + 2000.0),
                          0.0);
            pos = gatePos + offset;
            posSet = true;
            arriveGateID = id;
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
    // W-space systems have no stargates and usually no stations — land the bot
    // on a random orbit around the first planet/moon instead of the system
    // centre (0,0,0 = the sun), which triggered a SetPosition traceStack dump.
    if (!posSet) {
        for (auto& [id, se] : pSystem->GetStaticEntities()) {
            if (se != nullptr && (se->IsPlanetSE() || se->IsMoonSE())) {
                double ang = MakeRandomFloat() * 6.2831853;
                double rad = 8000.0 + MakeRandomFloat() * 20000.0;
                pos = se->GetPosition() + GPoint(cos(ang) * rad, sin(ang) * rad, (MakeRandomFloat() - 0.5) * 2000.0);
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

    // Give the bot's hull a combat profile. Real player ships (Raven etc.) don't
    // carry the NPC attack attributes (AttrEmDamage/AttrKineticDamage/...) that
    // NPC::constructor / NPCAI read — without them the bot locks targets but
    // deals ZERO damage. Set a class-based profile scaled by skill tier.
    {
        float base = 6.0f + (float)skillTier * 4.0f;   // 6..26 base DPS-ish
        uint16 grp = iRef->groupID();
        // Mining barges/exhumers (Retriever, Covetor, Procurer...) have no guns —
        // a barge "shooting" with a mining laser must not deal combat damage.
        // Peaceful pilots on real combat hulls still fight back with their weapons.
        if (grp == EVEDB::invGroups::MiningBarge || grp == EVEDB::invGroups::Exhumer)
            base = 0.0f;
        // Bigger hulls hit harder (battleship > cruiser > frigate).
        if (grp == EVEDB::invGroups::Battleship || grp == EVEDB::invGroups::BlackOps
            || grp == EVEDB::invGroups::Marauder)
            base *= 5.0f;
        else if (grp == EVEDB::invGroups::Battlecruiser || grp == EVEDB::invGroups::CommandShip
                 || grp == EVEDB::invGroups::StrategicCruiser)
            base *= 3.0f;
        else if (grp == EVEDB::invGroups::Cruiser || grp == EVEDB::invGroups::HeavyAssaultShip
                 || grp == EVEDB::invGroups::CombatRecon || grp == EVEDB::invGroups::Logistics)
            base *= 2.0f;
        if (!iRef->HasAttribute(AttrEmDamage))          iRef->SetAttribute(AttrEmDamage,         base * 0.4f, false);
        if (!iRef->HasAttribute(AttrKineticDamage))     iRef->SetAttribute(AttrKineticDamage,    base,         false);
        if (!iRef->HasAttribute(AttrThermalDamage))     iRef->SetAttribute(AttrThermalDamage,    base * 0.8f, false);
        if (!iRef->HasAttribute(AttrExplosiveDamage))   iRef->SetAttribute(AttrExplosiveDamage,  base * 0.2f, false);
        if (!iRef->HasAttribute(AttrDamageMultiplier))  iRef->SetAttribute(AttrDamageMultiplier, 2.0f, false);
        if (!iRef->HasAttribute(AttrSpeed))             iRef->SetAttribute(AttrSpeed,            (float)MakeRandomInt(2500, 5000), false);   // weapon cycle ms
        if (!iRef->HasAttribute(AttrMaxRange))          iRef->SetAttribute(AttrMaxRange,         15000.0f, false);   // optimal
        if (!iRef->HasAttribute(AttrFalloff))           iRef->SetAttribute(AttrFalloff,          10000.0f, false);
        if (!iRef->HasAttribute(AttrTrackingSpeed))     iRef->SetAttribute(AttrTrackingSpeed,    0.08f, false);
        if (!iRef->HasAttribute(AttrEntityFlyRange))    iRef->SetAttribute(AttrEntityFlyRange,   15000.0f, false);   // orbit range
        if (!iRef->HasAttribute(AttrEntityCruiseSpeed)) iRef->SetAttribute(AttrEntityCruiseSpeed, 180.0f, false);
        if (!iRef->HasAttribute(AttrOptimalSigRadius))  iRef->SetAttribute(AttrOptimalSigRadius, 40.0f, false);
        if (!iRef->HasAttribute(AttrSignatureRadius))   iRef->SetAttribute(AttrSignatureRadius,  iRef->GetAttribute(AttrRadius).get_float() * 5.0f, false);

        // Weapon type per hull — the attack effect must match the ship. The client
        // (spaceObject/entityShip.py) builds the turret model from the hull's
        // gfxTurretID (attribute 245) = the TYPE ID of a real turret/launcher
        // module. So we set AttrGfxTurretID to a T1 weapon module typeID matching
        // the hull's race + class:
        //   races: 1=Caldari(missiles), 4=Amarr(laser), 8=Gallente(hybrid), 2=Minmatar(projectile)
        // Missile boats also get AttrEntityMissileTypeID so NPCAI launches real
        // missiles (MissileDeployment effect + flying missile). Drone-capable hulls
        // (Vexor/Myrmidon/Dominix) field actual drones instead of shooting.
        uint16 raceID = 0;
        {
            Inv::TypeData tdata;
            sDataMgr.GetType((uint16)hullType, tdata);
            if (tdata.id == hullType)
                raceID = tdata.race;
        }
        // Miners carry a real Mining Laser I (483) so the client renders a mining
        // beam on the hull (group 54 = Mining Laser is in turretModuleGroups) —
        // same as a player miner's fit. Everyone else gets their race weapon.
        if (prof == PlayerBot::BotProfession::Miner) {
            if (!iRef->HasAttribute(AttrGfxTurretID))
                iRef->SetAttribute(AttrGfxTurretID, 483, false);   // Miner I
        } else {
            bool isMissileBoat = (raceID == 1)   // Caldari hulls are missile boats
                && (grp == EVEDB::invGroups::Cruiser || grp == EVEDB::invGroups::Battleship
                    || grp == EVEDB::invGroups::Battlecruiser);
            if (isMissileBoat) {
                // A real missile per hull class: light for cruisers, heavy for BC, cruise for BS.
                uint16 missileType = 210;    // Scourge Light Missile
                uint32 launcherType = 499;   // Light Missile Launcher I
                if (grp == EVEDB::invGroups::Battleship) {
                    missileType = 203;       // Scourge Cruise Missile
                    launcherType = 13320;    // Cruise Missile Launcher I
                } else if (grp == EVEDB::invGroups::Battlecruiser) {
                    missileType = 209;       // Scourge Heavy Missile
                    launcherType = 501;      // Heavy Missile Launcher I
                }
                if (!iRef->HasAttribute(AttrEntityMissileTypeID)) {
                    iRef->SetAttribute(AttrEntityMissileTypeID, missileType, false);
                    iRef->SetAttribute(AttrMissileLaunchDuration, 5000.0f, false);
                }
                if (!iRef->HasAttribute(AttrGfxTurretID))
                    iRef->SetAttribute(AttrGfxTurretID, launcherType, false);
            } else if (raceID != 0) {
                // Turret boats: the right weapon module typeID per race. Note drone
                // hulls (Vexor/Myrmidon/Dominix etc.) ALSO fit turrets — EVE ships
                // carry both weapon systems. Drones are handled separately via the
                // hull's AttrDroneCapacity in PlayerBot::GetDroneCapacity/SpawnDrones.
                uint32 turretType = 450;     // default: Amarr Gatling Pulse Laser I
                if (raceID == 8)      turretType = 561;    // Gallente 75mm Gatling Rail I (hybrid)
                else if (raceID == 2) turretType = 484;    // Minmatar 125mm Gatling AutoCannon I
                if (!iRef->HasAttribute(AttrGfxTurretID))
                    iRef->SetAttribute(AttrGfxTurretID, turretType, false);
            }
        }
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

    // Arrival animation: the bot "jumped through" the gate it spawned beside, so
    // play the gate flash to everyone in the bubble — otherwise a chelobot just
    // materialises out of nowhere (a tell that it's not a real pilot).
    if (arrivedViaGate && arriveGateID != 0 && bot->DestinyMgr() != nullptr) {
        bot->DestinyMgr()->SendGateActivity(arriveGateID);
        _log(BOT__TRACE, "BotMgr: %s(%u) gate-arrival animation at gate %u.",
             bot->GetBotName().c_str(), bot->GetBotCharID(), arriveGateID);
    }

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

    // EWAR fit per combat role — NPCAI reads these attributes and applies
    // web/scram/ECM/paint automatically in AttackTarget. Support ships are the
    // electronic-warfare specialists (jam + paint + web + scram); every fighter
    // carries a light scram so the fleet can hold targets (tackle).
    {
        uint8 role = (uint8)bot->GetRole();
        if (role == (uint8)PlayerBot::BotRole::Support) {
            if (!iRef->HasAttribute(AttrWarpScrambleRange))          iRef->SetAttribute(AttrWarpScrambleRange,         18000.0f, false);
            if (!iRef->HasAttribute(AttrWarpScrambleStrength))       iRef->SetAttribute(AttrWarpScrambleStrength,      2.0f,     false);
            if (!iRef->HasAttribute(AttrEntityWarpScrambleChance))   iRef->SetAttribute(AttrEntityWarpScrambleChance,  0.55f,    false);   // ~45% chance
            if (!iRef->HasAttribute(AttrModifyTargetSpeedRange))     iRef->SetAttribute(AttrModifyTargetSpeedRange,    22000.0f, false);   // stasis web
            if (!iRef->HasAttribute(AttrEntityTargetJamMaxRange))    iRef->SetAttribute(AttrEntityTargetJamMaxRange,   22000.0f, false);   // ECM
            if (!iRef->HasAttribute(AttrEntityTargetJam))            iRef->SetAttribute(AttrEntityTargetJam,           3.0f,     false);
            if (!iRef->HasAttribute(AttrEntityTargetJamDurationChance)) iRef->SetAttribute(AttrEntityTargetJamDurationChance, 0.5f, false);
            if (!iRef->HasAttribute(AttrEntityTargetJamDuration))    iRef->SetAttribute(AttrEntityTargetJamDuration,   10000.0f, false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintMaxRange))  iRef->SetAttribute(AttrEntityTargetPaintMaxRange, 25000.0f, false);   // target painter
            if (!iRef->HasAttribute(AttrEntityTargetPaintMultiplier)) iRef->SetAttribute(AttrEntityTargetPaintMultiplier, 0.25f, false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintDurationChance)) iRef->SetAttribute(AttrEntityTargetPaintDurationChance, 0.6f, false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintDuration))  iRef->SetAttribute(AttrEntityTargetPaintDuration, 10000.0f, false);
            _log(BOT__TRACE, "BotMgr: %s(%u) fitted full EWAR (web/scram/ECM/paint).",
                 bot->GetBotName().c_str(), bot->GetBotCharID());
        } else if (role == (uint8)PlayerBot::BotRole::Fighter) {
            if (!iRef->HasAttribute(AttrWarpScrambleRange))          iRef->SetAttribute(AttrWarpScrambleRange,         12000.0f, false);   // tackle scram
            if (!iRef->HasAttribute(AttrWarpScrambleStrength))       iRef->SetAttribute(AttrWarpScrambleStrength,      1.0f,     false);
            if (!iRef->HasAttribute(AttrEntityWarpScrambleChance))   iRef->SetAttribute(AttrEntityWarpScrambleChance,  0.75f,    false);   // ~25% chance
            // Light target painter — bigger sig = the fleet's guns/missiles hit harder.
            if (!iRef->HasAttribute(AttrEntityTargetPaintMaxRange))      iRef->SetAttribute(AttrEntityTargetPaintMaxRange,     20000.0f, false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintMultiplier))    iRef->SetAttribute(AttrEntityTargetPaintMultiplier,   0.15f,    false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintDurationChance)) iRef->SetAttribute(AttrEntityTargetPaintDurationChance, 0.5f,   false);
            if (!iRef->HasAttribute(AttrEntityTargetPaintDuration))      iRef->SetAttribute(AttrEntityTargetPaintDuration,     10000.0f, false);
        }
    }

    // Assign a combat style: most fight balanced (orbit at weapon range), some
    // kite (keep distance, chip away), some brawl (close in and scrap).
    {
        float r = MakeRandomFloat();
        if (r < 0.30f)
            bot->SetCombatStyle(PlayerBot::CombatStyle::Kite);
        else if (r < 0.55f)
            bot->SetCombatStyle(PlayerBot::CombatStyle::Brawler);
        else
            bot->SetCombatStyle(PlayerBot::CombatStyle::Balanced);
        _log(BOT__TRACE, "BotMgr: %s(%u) combat style = %u.", bot->GetBotName().c_str(), bot->GetBotCharID(), (uint8)bot->GetCombatStyle());
    }

    // Assign the profession decided earlier (before corp selection).
    bot->SetProfession(prof);
    _log(BOT__TRACE, "BotMgr: %s(%u) profession = %u.", bot->GetBotName().c_str(), bot->GetBotCharID(), (uint8)prof);

    // A minority of hunters are "faction warriors": FW-style militia that treats
    // bots of OTHER factions as their fixed enemies (subclass stub for future
    // faction-warfare content). It's just hunting with a filter on the enemy set.
    if (prof == PlayerBot::BotProfession::Hunter && bot->GetFaction() != 0
        && MakeRandomInt(0, 99) < 30) {
        bot->SetFactionWarrior(true);
        _log(BOT__TRACE, "BotMgr: %s(%u) is a faction warrior (faction %d).",
             bot->GetBotName().c_str(), bot->GetBotCharID(), bot->GetFaction());
    }

    // Join the system's local channel so the bot shows up in local chat.
    // W-space local is intentionally hidden (no member list per EVE lore), so
    // bots there stay visible only in space, never in the channel counter.
    if (sConfig.playerBots.Enabled && !IsWSpaceID(pSystem->GetID())) {
        LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
        if (lsc != nullptr) {
            LSCChannel* chan = lsc->GetChannelByID((int32)pSystem->GetID());
            if (chan != nullptr)
                chan->AddBotChar(useCharID, useCorpID, useAllianceID, 0, useName);
        }
    }
}

void BotMgr::FetchPortraitAsync(uint32 serverCharID, uint32 eveCharID)
{
    // Download the bot's ESI portrait into the image cache right now, so the
    // client shows a face immediately (no cron lag). Runs curl in a forked child
    // so the game loop never blocks. Path: <imageDir>/Character/<serverCharID>_512.jpg
    // (ImageServer::GetFilePath). We write exactly what fetch_bot_portraits.py would.
    if (serverCharID == 0 || eveCharID == 0)
        return;

    std::string base = sConfig.files.imageDir;
    if (!base.empty() && base[base.size() - 1] != '/')
        base += "/";
    std::string dir = base + "Character/";
    std::string path = dir + std::to_string(serverCharID) + "_512.jpg";

    // Skip if the portrait already exists.
    struct stat st;
    if (::stat(path.c_str(), &st) == 0 && st.st_size > 0)
        return;

    ::mkdir(dir.c_str(), 0755);

    // ESI portrait endpoint: https://images.evetech.net/characters/{eveID}/portrait?size=512
    std::string url = "https://images.evetech.net/characters/"
                    + std::to_string(eveCharID) + "/portrait?size=512";

    pid_t pid = ::fork();
    if (pid == 0) {
        // child: curl -sSL --max-time 15 <url> -o <path>
        ::execlp("curl", "curl", "-sSL", "--max-time", "15", url.c_str(), "-o", path.c_str(), (char*)nullptr);
        _exit(127);
    }
    // parent: don't wait — let it finish in the background
    _log(BOT__TRACE, "BotMgr: fetching portrait for bot %u (eve %u) -> %s", serverCharID, eveCharID, path.c_str());
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
            if (pb->IsAggressed())
                continue;   // aggression timer — can't jump a gate until it cools down
            if (pb->WantsToTravel())
                readyToJump.push_back(pb);   // visible flight to the gate is done
            else if (MakeRandomInt(0, 299) == 0)   // ~0.33% per tic decides to leave
                pb->MarkForTravel();               // starts the visible warp
        }

        for (PlayerBot* pb : readyToJump) {
            // Cross the gate. Use the bot's requested destination if set
            // (e.g. arriving into a player's system), else pick a random one.
            uint32 destSystem = pb->GetTravelDestination();
            if (destSystem == 0) {
                // Trade-inclined bots (traders, couriers, miners hauling ore)
                // route toward the primary market hub (Jita) to sell/buy.
                uint32 hub = GetTradeHubSystem();
                bool wantsHub = (pb->GetProfession() == PlayerBot::BotProfession::Trader
                              || pb->GetProfession() == PlayerBot::BotProfession::Courier
                              || pb->GetProfession() == PlayerBot::BotProfession::Miner
                              || pb->GetProfession() == PlayerBot::BotProfession::Explorer);
                if (wantsHub && hub != 0 && hub != pSystem->GetID() && MakeRandomInt(0, 99) < 70)
                    destSystem = hub;
                else
                    destSystem = GetRandomAdjacentSystem(pSystem->GetID());
            }
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
                SpawnBot(dest, charID, name, corp, ally, true);   // arrived via gate → jump-in animation
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

uint32 BotMgr::GetTradeHubSystem() const
{
    // Primary market hub (Jita by default, from botTradeHubs).
    static uint32 cachedHub = 0;
    if (cachedHub != 0)
        return cachedHub;
    DBQueryResult res;
    if (sDatabase.RunQuery(res, "SELECT systemID FROM botTradeHubs WHERE isPrimary = 1 LIMIT 1")) {
        DBResultRow row;
        if (res.GetRow(row))
            cachedHub = row.GetUInt(0);
    }
    return cachedHub;
}

bool BotMgr::IsTradeHub(uint32 systemID) const
{
    if (systemID == 0)
        return false;
    DBQueryResult res;
    if (sDatabase.RunQuery(res, "SELECT systemID FROM botTradeHubs WHERE systemID = %u", systemID)) {
        DBResultRow row;
        if (res.GetRow(row))
            return true;
    }
    return false;
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
        "  ('%s', '', '%s', '', 0.1, 2, 1,"
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
        " VALUES ('%s', '%s', '', %u, %u, %u, %f, %u, '')",
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
    // Market work happens DOCKED, not in space — a pilot can't place a sell order
    // while flying. Space bots just pay corp tax here; docked traders/producers
    // place orders in ProcessDockedEconomy (which knows their station).
    if (bot == nullptr || !sConfig.playerBots.Enabled)
        return;
    if (MakeRandomInt(0, 999) < 30)
        PayCorpTax(bot);
}

void BotMgr::ProcessDockedEconomy()
{
    // Docked traders/producers work the market FROM THEIR STATION: sell orders,
    // buy orders and the occasional courier contract. A bot in space can't do
    // this (it's flying), so we iterate the docked list, not the space entities.
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    if (m_docked.empty())
        return;

    for (auto& [sysID, vec] : m_docked) {
        if (sysID == 0 || vec.empty())
            continue;
        for (auto& db : vec) {
            // Throttle: a trader works the market every few minutes, not every tick.
            time_t now = time(nullptr);
            auto lt = m_lastTrade.find(db.charID);
            if (lt != m_lastTrade.end() && (now - lt->second) < MakeRandomInt(240, 600))
                continue;
            m_lastTrade[db.charID] = now;

            uint8 prof = db.profession;
            if (prof == (uint8)PlayerBot::BotProfession::Trader) {
                // Traders: sell + buy (play the spread) + occasional courier job.
                PlaceBotOrderAt(sysID, db.charID, db.corpID);
                PlaceBotBuyOrderAt(sysID, db.charID, prof);
                PlaceBotCourierContractAt(sysID, db.charID, db.corpID);
            } else if (prof == (uint8)PlayerBot::BotProfession::Miner
                       || prof == (uint8)PlayerBot::BotProfession::Hacker
                       || prof == (uint8)PlayerBot::BotProfession::Explorer) {
                // Producers bid for the raw materials they consume (from the dock).
                if (MakeRandomInt(0, 99) < 20)
                    PlaceBotBuyOrderAt(sysID, db.charID, prof);
            }
        }
    }
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
    // Trader bots sell goods on the market in their own name (legacy space-bot
    // entry; the docked path uses PlaceBotOrderAt). Orders are real mktOrders
    // rows (visible to players).
    if (bot == nullptr)
        return;
    uint32 sysID = bot->SystemMgr() ? bot->SystemMgr()->GetID() : 0;
    if (sysID == 0)
        return;
    PlaceBotOrderAt(sysID, bot->GetBotCharID(), bot->GetBotCorpID());
}

void BotMgr::PlaceBotOrderAt(uint32 sysID, uint32 charID, uint32 corpID)
{
    if (sysID == 0 || charID == 0)
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
         "trader", charID, qty, typeID, price, sysID);
}

void BotMgr::PlaceBotBuyOrder(PlayerBot* bot)
{
    // Buy orders work the other way: producers bid for raw materials they need
    // (ore, minerals, ammo) and traders buy low to resell high. Orders are real
    // mktOrders rows with bid=1, visible to players like any market order.
    if (bot == nullptr)
        return;
    uint32 sysID = bot->SystemMgr() ? bot->SystemMgr()->GetID() : 0;
    if (sysID == 0)
        return;
    PlaceBotBuyOrderAt(sysID, bot->GetBotCharID(), (uint8)bot->GetProfession());
}

void BotMgr::PlaceBotBuyOrderAt(uint32 sysID, uint32 charID, uint8 profession)
{
    if (sysID == 0 || charID == 0)
        return;

    // Producers (miners/industrials) buy the goods they consume; traders buy
    // whatever they think is underpriced (play the spread).
    static const uint32 rawMats[]  = { 34, 38, 39, 40, 1229, 1230, 1231, 1232, 2048, 2488 };   // trit/pye/mex/iso, minerals
    static const uint32 tradeGoods[] = { 3775, 2676, 2048, 1229, 1230, 38, 39, 40 };            // ammo, minerals, common
    uint32 typeID;
    if (profession == (uint8)PlayerBot::BotProfession::Trader)
        typeID = tradeGoods[MakeRandomInt(0, 7)];
    else
        typeID = rawMats[MakeRandomInt(0, 9)];

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

    // Buy orders sit below the market price (a trader buys cheap). Producers bid
    // a bit higher so they actually get the ore.
    double price = 50.0 + (MakeRandomInt(0, 40000) / 100.0);
    if (profession != (uint8)PlayerBot::BotProfession::Trader)
        price *= 1.6;   // producers pay more for what they need
    uint32 qty = MakeRandomInt(100, 2000);

    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO mktOrders"
        "  (typeID, ownerID, regionID, stationID, solarSystemID, orderRange, bid, price,"
        "   escrow, minVolume, volEntered, volRemaining, issued, duration, isCorp, accountKey, memberID)"
        " VALUES"
        "  (%u, %u, (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u, 32767, 1, %f,"
        "   %f, 1, %u, %u, %f, 90, 0, 1000, %u)",
        typeID, charID, sysID, stationID, sysID, price, price * qty, qty, qty, GetFileTimeNow(), charID);
    _log(BOT__TRACE, "BotMgr: trader(%u) placed buy order %ux type %u @ %.2f ISK in %u.",
         charID, qty, typeID, price, sysID);
}

void BotMgr::PlaceBotCourierContract(PlayerBot* bot)
{
    // A trader occasionally lists a PUBLIC courier contract for one of its
    // "shipments" between two stations (legacy space-bot entry; docked path uses
    // PlaceBotCourierContractAt).
    if (bot == nullptr || bot->GetProfession() != PlayerBot::BotProfession::Trader)
        return;
    if (MakeRandomInt(0, 999) >= 20)
        return;   // ~2% per economy tick — rare
    uint32 sysID = bot->SystemMgr() ? bot->SystemMgr()->GetID() : 0;
    if (sysID == 0)
        return;
    PlaceBotCourierContractAt(sysID, bot->GetBotCharID(), bot->GetBotCorpID());
}

void BotMgr::PlaceBotCourierContractAt(uint32 sysID, uint32 charID, uint32 corpID)
{
    if (sysID == 0 || charID == 0)
        return;
    if (MakeRandomInt(0, 999) >= 20)
        return;   // ~2% per economy tick — rare

    // Start station (where the trader is) and a random other station as the
    // destination — the courier "hauls goods to the hub".
    DBQueryResult res;
    uint32 startStation = 0, endStation = 0, endSys = 0;
    if (!sDatabase.RunQuery(res, "SELECT stationID FROM staStations WHERE solarSystemID = %u LIMIT 1", sysID))
        return;
    DBResultRow srow;
    if (!res.GetRow(srow))
        return;
    startStation = srow.GetUInt(0);

    // Destination: the primary trade hub, or any random station elsewhere.
    uint32 hub = GetTradeHubSystem();
    if (hub != 0 && hub != sysID && MakeRandomInt(0, 99) < 70) {
        if (!sDatabase.RunQuery(res, "SELECT stationID FROM staStations WHERE solarSystemID = %u LIMIT 1", hub))
            return;
        if (res.GetRow(srow)) {
            endStation = srow.GetUInt(0);
            endSys = hub;
        }
    }
    if (endStation == 0) {
        if (!sDatabase.RunQuery(res, "SELECT stationID, solarSystemID FROM staStations ORDER BY RAND() LIMIT 1"))
            return;
        if (res.GetRow(srow)) {
            endStation = srow.GetUInt(0);
            endSys = srow.GetUInt(1);
        }
    }
    if (endStation == 0 || endStation == startStation)
        return;

    // Modest cargo and reward — enough to be worth a courier's time but not a
    // jackpot a player would hoard. Larger = more visible on the market.
    double volume = 200.0 + MakeRandomFloat() * 4000.0;
    int64 reward = (int64)(50000 + volume * 40.0);

    DBerror err;
    if (!sDatabase.RunQuery(err,
        "INSERT INTO ctrContracts"
        "  (contractType, issuerID, issuerCorpID, forCorp, isPrivate, assigneeID,"
        "   dateIssued, dateExpired, duration, numDays, startStationID, startSolarSystemID,"
        "   startRegionID, endStationID, endSolarSystemID, endRegionID, price, reward, collateral,"
        "   title, description, status, volume)"
        " VALUES"
        "  (3, %u, %u, 0, 0, 0, %lli, %lli, 7, 7, %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), 0, %lli, 0,"
        "   'Courier shipment', 'Standard courier contract', 0, %f)",
        charID, corpID,
        (int64)GetFileTimeNow(), (int64)GetFileTimeNow() + 7LL * EvE::Time::Day,
        startStation, sysID, sysID, endStation, endSys, endSys, reward, volume))
    {
        _log(BOT__MESSAGE, "BotMgr: trader %u issued courier contract (%.0f m3, reward %.0f ISK) %u -> %u.",
             charID, volume, (double)reward, sysID, endSys);
    }
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
            // The bot just "undocked": place it beside the station (not at the
            // gate) so it's scannable right there. Then it behaves like a real
            // pilot: couriers/traders head out through a gate on business, while
            // producers (miners/ratters/hackers/explorers) go work — mine, scan,
            // rat — near the station or at an anomaly. No mid-space teleport.
            for (auto& [uid, use] : pSystem->GetEntities()) {
                if (use == nullptr || use->GetNPCSE() == nullptr)
                    continue;
                PlayerBot* npb = dynamic_cast<PlayerBot*>(use->GetNPCSE());
                if (npb == nullptr || npb->GetBotCharID() != db->charID)
                    continue;
                SystemEntity* station = nullptr;
                for (auto& [sid, sse] : pSystem->GetStaticEntities()) {
                    if (sse != nullptr && sse->GetStationSE() != nullptr) { station = sse; break; }
                }
                if (station != nullptr) {
                    double sr = station->GetRadius() > 500.0 ? station->GetRadius() : 2000.0;
                    npb->DestinyMgr()->SetPosition(station->GetPosition() + GPoint(sr + 5000.0, 0, 0));
                }
                npb->ClearDockRequest();   // just undocked — don't immediately re-dock
                // Travellers (courier/trader) leave via the gate on business;
                // producers stay and work near the station (their profession will
                // warp them to a belt/anomaly/site).
                auto prof = npb->GetProfession();
                if (prof == PlayerBot::BotProfession::Courier || prof == PlayerBot::BotProfession::Trader)
                    npb->MarkForTravel();   // visible warp to the gate, then cross
                break;
            }
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
    //    at the market) fly to the station first (visible approach), then dock
    //    when they get there; others dock occasionally.
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
            if (pb->IsAggressed())
                continue;   // aggression timer — can't dock mid-aggression
            if (pb->GetAIMgr()->IsFighting())
                continue;   // in combat — never vanish mid-fight (no teleport)
            if (!pb->WantsDock() && MakeRandomInt(0, 599) != 0)
                continue;   // neither profession wants the station nor occasional roll
            // Fly to the station first so the dock is visible (no teleport).
            SystemEntity* station = nullptr;
            for (auto& [sid, sse] : pSystem->GetStaticEntities()) {
                if (sse != nullptr && sse->GetStationSE() != nullptr) { station = sse; break; }
            }
            if (station == nullptr)
                continue;   // no station in this system — don't pop the bot out of space
            {
                double stationR = station->GetRadius() > 500.0 ? station->GetRadius() : 2000.0;
                double dist = pb->GetPosition().distance(station->GetPosition());
                double approachDist = stationR + 15000.0;
                if (dist > approachDist) {
                    if (!pb->DestinyMgr()->IsWarping() && !pb->GetAIMgr()->IsFighting())
                        pb->DestinyMgr()->WarpTo(station->GetPosition(), (int32)(stationR + 5000.0));
                    continue;   // not at the station yet — retry next tic
                }
            }
            toDock.push_back(pb);
        }
        for (PlayerBot* pb : toDock) {
            DockedBot db;
                db.charID = pb->GetBotCharID();
                db.name = pb->GetBotName();
                db.corpID = pb->GetBotCorpID();
                db.allianceID = pb->GetBotAllianceID();
                db.profession = (uint8)pb->GetProfession();
                // Traders/market guys sit longer; miners refine/sell quickly then head out.
                db.undockAt = now + (pb->GetProfession() == PlayerBot::BotProfession::Trader
                                     ? MakeRandomInt(300, 1800)    // 5-30 min at market
                                     : MakeRandomInt(30, 300));    // 0.5-5 min for everyone else
            m_docked[pSystem->GetID()].push_back(db);
            _log(BOT__MESSAGE, "BotMgr: %s(%u) docking at station in system %u.",
                 db.name.c_str(), db.charID, pSystem->GetID());
            pb->ClearDockRequest();
            pb->RecallDrones();   // scoop drones before docking
            pb->Delete();   // remove from space; stays in local channel as docked
        }
    }
}

void BotMgr::ProcessBotSmalltalk()
{
    // Simulated players occasionally talk to each other in local. Real servers
    // have constant low-level chatter; ours should too — but rarely enough that
    // it doesn't spam the channel or hit the DeepSeek API. Uses canned, natural
    // EVE-ish lines so it stays believable and free.
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    if (sEntityList.GetSystems().empty())
        return;

    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr || pSystem->PlayerCount() < 1)
            continue;

        // Throttle: at most one bot-to-bot line per system per ~4 minutes.
        time_t now = time(nullptr);
        auto last = m_lastSmalltalk.find((int32)sysID);
        if (last != m_lastSmalltalk.end() && (now - last->second) < 240)
            continue;

        // Need at least two bots in space to talk.
        std::vector<PlayerBot*> present;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr)
                present.push_back(pb);
        }
        if (present.size() < 2)
            continue;

        // Rare: ~15% chance per eligible system per tick window.
        if (MakeRandomInt(0, 99) >= 15)
            continue;
        m_lastSmalltalk[(int32)sysID] = now;

        PlayerBot* a = present[MakeRandomInt(0, (int64)present.size() - 1)];
        PlayerBot* b = present[MakeRandomInt(0, (int64)present.size() - 1)];
        if (a == nullptr || b == nullptr || a == b)
            continue;

        static const char* lines[] = {
            "anyone found a good belt here?",
            "gate to %s is clear, ganked anyone today?",
            "just lost my hauler to a smartbomb. classic.",
            "market prices are crazy today.",
            "anyone wanna fleet up?",
            "this system is dead, moving on.",
            "that last ratting run paid off.",
            "has anyone seen a wormhole in here?",
            "solo PvP or nothing.",
            "the ISK is in the haul, not the fight.",
            "someone left a wreck at the gate. free loot?",
            "my drones keep orbiting the wrong asteroid lol",
            "wh trade route is juicy today.",
            "docking in a sec, brb.",
            "anyone else hear that anoms got buffed?",
        };

        std::string msg = lines[MakeRandomInt(0, 14)];
        // The one phrase with a placeholder: "gate to %s is clear..." — substitute
        // the actual system name so it reads naturally.
        {
            std::string sysName = pSystem->GetName();
            size_t pos = msg.find("%s");
            if (pos != std::string::npos)
                msg.replace(pos, 2, sysName.empty() ? "the gate" : sysName);
        }
        // Sometimes address the other bot by name.
        if (MakeRandomInt(0, 99) < 40)
            msg = std::string(b->GetBotName().c_str()) + ", " + msg;

        LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
        if (lsc == nullptr)
            continue;
        LSCChannel* chan = lsc->GetChannelByID((int32)sysID);
        if (chan == nullptr)
            continue;
        chan->SendBotMessage(a->GetBotCharID(), a->GetBotName(), a->GetBotCorpID(), msg);
        RecordChannelPhrase((int32)sysID, a->GetBotCharID(), msg);
        _log(BOT__MESSAGE, "BotMgr: %s(%u) said to %s in local: %s",
             a->GetBotName().c_str(), a->GetBotCharID(), b->GetBotName().c_str(), msg.c_str());
    }
}

void BotMgr::RecordChannelPhrase(int32 channelID, uint32 charID, const std::string& phrase)
{
    // Keep the last 10 lines per channel, drop anything older than 2 minutes.
    time_t now = time(nullptr);
    auto& q = m_channelPhrases[channelID];
    q.push_back({ charID, phrase, now });
    while (q.size() > 10 || (!q.empty() && (now - q.front().when) > 120))
        q.pop_front();
}

void BotMgr::ProcessBotReplies()
{
    // Drain ONE queued bot line per tic: the bot that should reply answers now,
    // and its own answer re-queues the next reaction, so a bot<-bot conversation
    // advances one line per tick. This is what makes chat alive WITHOUT recursing
    // through the stack (nested HandleLocalMessage/SendBotMessage used to overflow
    // it -> SIGSEGV). A human line in a channel still starts/reset a chain.
    if (m_pendingBotReplies.empty())
        return;
    PendingBotReply r = m_pendingBotReplies.front();
    m_pendingBotReplies.erase(m_pendingBotReplies.begin());
    if (!m_initalized || !sConfig.playerBots.Enabled || !sConfig.playerBots.ChatEnabled)
        return;
    HandleLocalMessage(r.channelID, r.charID, r.name, r.message);
}

void BotMgr::ProcessPlayerContracts()
{
    // Courier bots take over player courier contracts that nobody accepted.
    // A contract that has been sitting unaccepted (issued > 5 min ago) is
    // picked up by a free courier bot, who then flies it to the destination.
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    if (sEntityList.GetSystems().empty())
        return;

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT contractId, startStationID, startSolarSystemID, endSolarSystemID, reward, dateIssued, volume"
        " FROM ctrContracts"
        " WHERE contractType = 3"        // courier
        "   AND status = 0"              // created, not yet accepted
        "   AND acceptorID = 0"          // nobody picked it up
        "   AND isPrivate = 0"           // public contract
        " LIMIT 10"))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            uint32 contractID = row.GetUInt(0);
            uint32 startSys = row.GetUInt(2);
            uint32 endSys = row.GetUInt(3);
            int64 reward = row.GetInt64(4);
            int64 dateIssued = row.GetInt64(5);
            double volume = row.GetDouble(6);
            // Skip contracts that were issued recently — only take ones that
            // have been sitting unaccepted for a while (a real player may still
            // pick up a fresh one). dateIssued is FILETIME (100ns ticks).
            if (dateIssued > 0 && (GetFileTimeNow() - dateIssued) < 5LL * EvE::Time::Minute)
                continue;

            // Find a free courier bot (in the contract's start system if loaded).
            SystemManager* startSysMgr = sEntityList.IsSystemLoaded(startSys) ? sEntityList.FindOrBootSystem(startSys) : nullptr;
            PlayerBot* courier = nullptr;
            if (startSysMgr != nullptr) {
                for (auto& [id, se] : startSysMgr->GetEntities()) {
                    if (se == nullptr || se->GetNPCSE() == nullptr)
                        continue;
                    PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
                    if (pb != nullptr && pb->GetProfession() == PlayerBot::BotProfession::Courier
                        && !pb->WantsToTravel() && !pb->IsTraveling() && !pb->WantsDock()) {
                        courier = pb;
                        break;
                    }
                }
            }
            if (courier == nullptr)
                continue;   // no free courier right now — leave contract for later

            // Accept the contract: mark acceptorID and status.
            DBerror err;
            sDatabase.RunQuery(err,
                "UPDATE ctrContracts SET acceptorID = %u, status = 1, dateAccepted = %lli WHERE contractId = %u",
                courier->GetBotCharID(), (int64)GetFileTimeNow(), contractID);

            _log(BOT__MESSAGE, "BotMgr: courier %s(%u) accepted contract %u (reward %.0f ISK, %.0f m3) to system %u.",
                 courier->GetBotName().c_str(), courier->GetBotCharID(), contractID, (double)reward, volume, endSys);

            if (endSys != 0) {
                // Big cargo (>10,000 m3) goes by JUMP FREIGHTER through a cyno —
                // lights a visible cyno, holds an interception window (players can
                // warp in and shoot it or its guards), then jumps. Guards protect it.
                if (volume > 10000) {
                    courier->StartJumpFreighter(endSys);
                } else {
                    // Small cargo: fly through gates normally.
                    courier->SetTravelDestination(endSys);
                    courier->MarkForTravel(endSys);
                }
                // Reward ISK is paid on successful delivery (handled when the
                // freighter/courier completes the run), not at acceptance.
            }
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

    // Is the sender another bot (simulated player), not a real Client?
    // A real player ALWAYS has a Client object (charID >= minCharacter, same range
    // as bots). So the authoritative test is: no live Client => not a real player.
    // (The old `&& senderCharID >= 90000000` wrongly flagged real players whose
    // charID falls in the character range — they'd be treated as bots in chat.)
    bool senderIsBot = (sEntityList.FindClientByCharID(senderCharID) == nullptr);

    // Loop breaker: a bot-to-bot conversation must not echo forever. Track how
    // many consecutive lines were bots; once the chain is long enough, stop
    // reacting until a real player speaks again (a player line resets the chain).
    // Learned replies were so instant that bots ping-ponged the same phrases in
    // an infinite loop before this guard.
    uint32& chain = m_botChainDepth[channelID];
    if (!senderIsBot) {
        chain = 0;   // a real player's line starts a fresh exchange
    } else {
        if (chain >= 4)   // four bot lines in a row with no player input -> enough
            return;
        ++chain;
    }

    // Find a bot in that system (other than the sender — bots never message each
    // other's own ID here, but guard anyway). If the line ADDRESSES a specific
    // bot by name ("Name, ...", "@Name ...", "hey Name"), that bot replies; anyone
    // else may still jump in later. Otherwise the first other bot takes it.
    PlayerBot* responder = nullptr;
    bool addressed = false;
    std::string lowerMsg = message;
    std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
    for (auto& [id, se] : pSystem->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (pb == nullptr || pb->GetBotCharID() == senderCharID)
            continue;
        if (responder == nullptr)
            responder = pb;   // fallback — first other bot
        // Addressed by name? (case-insensitive). Guards against the sender's own
        // name matching, and against single-letter names matching inside words.
        std::string botNameLower = pb->GetBotName();
        std::transform(botNameLower.begin(), botNameLower.end(), botNameLower.begin(), ::tolower);
        if (botNameLower.size() >= 3 && lowerMsg.find(botNameLower) != std::string::npos) {
            responder = pb;
            addressed = true;
        }
    }
    if (responder == nullptr)
        return;

    // ---- 1) LEARNED phrases first (lively, no throttle) ----
    // Answer from phrases this bot learned (a line it said that drew a reply).
    // Reuse is immediate and frequent — a remembered exchange is "live". Only
    // when nothing learned matches do we fall back to DeepSeek (rare, throttled).
    // Match by shared words so "anyone know a good belt" reuses a reply learned
    // for "good belt here?".
    {
        std::string learnedReply;
        DBQueryResult lres;
        if (sDatabase.RunQuery(lres,
            "SELECT reply, UNIX_TIMESTAMP(lastUse) FROM botChatLearned WHERE charID = %u"
            " ORDER BY lastUse ASC LIMIT 50", responder->GetBotCharID()))
        {
            // Prefer a reply this bot hasn't used in 15-20 min (unique-ish lines);
            // fall back to the oldest match (a rare repeat) only if nothing fresh
            // AND it wasn't just used (>= 60s ago) — otherwise the same single
            // matching phrase would be re-said instantly on every trigger.
            std::string fallback;
            DBResultRow lrow;
            while (lres.GetRow(lrow)) {
                std::string cand = lrow.GetText(0);
                if (cand.empty())
                    continue;
                // Don't quote the exact same line back (no copy-paste replies).
                std::string candLower = cand;
                std::transform(candLower.begin(), candLower.end(), candLower.begin(), ::tolower);
                if (candLower == lowerMsg)
                    continue;
                // Count shared words between the incoming line and the learned reply.
                int overlap = 0;
                std::istringstream iss(lowerMsg);
                std::string w;
                while (iss >> w) {
                    if (w.size() < 4)
                        continue;   // skip short filler words
                    if (candLower.find(w) != std::string::npos)
                        ++overlap;
                }
                if (overlap < 1)
                    continue;
                // No-repeat guard: skip phrases any bot said in this channel within
                // the last 2 minutes. Every bot was seeded with the SAME phrase set,
                // so without this two bots bounce the identical lines forever
                // (X -> Y -> X via a shared word like "scam"/"it's").
                bool recent = false;
                auto chIt = m_channelPhrases.find(channelID);
                if (chIt != m_channelPhrases.end()) {
                    time_t now0 = time(nullptr);
                    for (const auto& bp : chIt->second) {
                        if ((now0 - bp.when) < 120 && bp.phrase == cand) {
                            recent = true;
                            break;
                        }
                    }
                }
                if (recent)
                    continue;
                time_t lastUse = (time_t)lrow.GetInt64(1);
                time_t now = time(nullptr);
                if (fallback.empty())
                    fallback = cand;
                if ((now - lastUse) >= 15 * 60) {
                    learnedReply = cand;   // a phrase not used in the last 15-20 min
                    break;
                }
            }
            if (learnedReply.empty() && !fallback.empty()) {
                // Rare repeat: reuse the oldest match only if it hasn't been said
                // in the last 60s. The 15-min window only guards the primary pick;
                // without a floor here a single matching phrase loops instantly.
                time_t fbLast = 0;
                bool fbFresh = false;
                std::string fbEsc;
                sDatabase.DoEscapeString(fbEsc, fallback);
                DBQueryResult fres;
                if (sDatabase.RunQuery(fres,
                    "SELECT UNIX_TIMESTAMP(lastUse) FROM botChatLearned"
                    " WHERE reply = '%s' AND charID = %u LIMIT 1",
                    fbEsc.c_str(), responder->GetBotCharID()))
                {
                    DBResultRow frow;
                    if (fres.GetRow(frow))
                        fbLast = (time_t)frow.GetInt64(0);
                }
                if (fbLast == 0 || (time(nullptr) - fbLast) >= 60)
                    learnedReply = fallback;
            }
        }
        if (!learnedReply.empty()) {
            // Reuse the learned reply (mark it used).
            std::string replyEsc;
            sDatabase.DoEscapeString(replyEsc, learnedReply);
            DBerror uerr;
            sDatabase.RunQuery(uerr,
                "UPDATE botChatLearned SET uses = uses + 1, lastUse = NOW()"
                " WHERE reply = '%s' AND charID = %u", replyEsc.c_str(), responder->GetBotCharID());
            LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
            if (lsc != nullptr) {
                LSCChannel* chan = lsc->GetChannelByID(channelID);
                if (chan != nullptr) {
                    chan->SendBotMessage(responder->GetBotCharID(), responder->GetBotName(),
                                         responder->GetBotCorpID(), learnedReply);
                    RecordChannelPhrase(channelID, responder->GetBotCharID(), learnedReply);
                    m_lastBotPhrase[channelID] = { responder->GetBotCharID(), learnedReply, time(nullptr) };
                    _log(BOT__MESSAGE, "BotMgr: %s(%u) reused learned reply: %s",
                         responder->GetBotName().c_str(), responder->GetBotCharID(), learnedReply.c_str());
                }
            }
            if (responder->GetMemory() != nullptr) {
                responder->GetMemory()->RecordChatLine();
                responder->GetMemory()->Save();
            }
            return;
        }
    }

    // ---- 2) DeepSeek fallback (throttled) ----
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

    // ---- minimal message analysis (so the bot actually responds to what was
    // said instead of spitting a canned line at every message) ----
    // Lowercased copy for keyword checks (keep the original for the prompt).
    std::string low = message;
    for (auto& c : low) c = (char)tolower((unsigned char)c);
    bool isQuestion = low.find('?') != std::string::npos
        || low.find("what") != std::string::npos || low.find("who") != std::string::npos
        || low.find("why") != std::string::npos || low.find("where") != std::string::npos
        || low.find("when") != std::string::npos || low.find("how") != std::string::npos
        || low.find("можно") != std::string::npos || low.find("как ") != std::string::npos
        || low.find("что") != std::string::npos || low.find("кто") != std::string::npos
        || low.find("почему") != std::string::npos || low.find("где") != std::string::npos
        || low.find("когда") != std::string::npos || low.find("почем") != std::string::npos;
    bool isGreeting = low.find("hi") != std::string::npos || low.find("hello") != std::string::npos
        || low.find("hey") != std::string::npos || low.find("yo ") != std::string::npos
        || low.find("привет") != std::string::npos || low.find("здравств") != std::string::npos
        || low.find("салют") != std::string::npos;
    bool isHelp = low.find("help") != std::string::npos || low.find("помощ") != std::string::npos
        || low.find("подскаж") != std::string::npos;
    bool isFleet = low.find("fleet") != std::string::npos || low.find("фит") != std::string::npos
        || low.find("группа") != std::string::npos || low.find("флот") != std::string::npos;
    bool isInsult = low.find("nub") != std::string::npos || low.find("noob") != std::string::npos
        || low.find("nooob") != std::string::npos || low.find("нуб") != std::string::npos
        || low.find("fuck") != std::string::npos || low.find("idiot") != std::string::npos;

    // Append the intent so the model answers ON TOPIC, not with a generic line.
    if (isQuestion) {
        prompt += " [This is a direct QUESTION — answer it properly and concretely,"
                  " on topic, as yourself. Do not dodge it with an unrelated remark.]";
    } else if (isGreeting) {
        prompt += " [This is a GREETING — greet them back naturally and briefly.]";
    } else if (isHelp) {
        prompt += " [They are asking for HELP/advice — give a short, useful, in-character answer.]";
    } else if (isFleet) {
        prompt += " [They mention a fleet/gang/fit — react as a pilot to that subject.]";
    } else if (isInsult) {
        prompt += " [They are INSULTING you — respond in character: dismissive, blunt or mocking,"
                  " but stay within EVE chat rules.]";
    } else {
        prompt += " [They made a casual statement — reply naturally to what was said,"
                  " on topic if possible; a short relevant remark is better than a random phrase.]";
    }
    // Reply in the SAME language the player wrote in (Russian, English, etc.) —
    // a real pilot from any country chats in their native tongue. The bot's
    // language/slang improves over time (chat self-learning: replies that drew
    // a response are "good").
    // Each bot has its OWN personality (profession + name), so replies aren't
    // interchangeable — a miner talks about ore, a hunter about ganks. Vary
    // wording; never repeat a stock phrase.
    std::string roleText = "an EVE pilot";
    switch (responder->GetProfession()) {
        case PlayerBot::BotProfession::Miner:     roleText = "a miner (mines asteroids in a barge, watches d-scan)"; break;
        case PlayerBot::BotProfession::Hunter:    roleText = "a PvP pirate (lives in lowsec/nullsec, hunts targets)"; break;
        case PlayerBot::BotProfession::RatHunter: roleText = "a ratter (farms NPC bounties in anomalies)"; break;
        case PlayerBot::BotProfession::Trader:    roleText = "a market trader (sits at a station working buy/sell orders)"; break;
        case PlayerBot::BotProfession::Courier:   roleText = "a courier/hauler (moves cargo between stations and systems)"; break;
        case PlayerBot::BotProfession::Hacker:    roleText = "a hacker/explorer (runs data and relic sites)"; break;
        case PlayerBot::BotProfession::Explorer:  roleText = "an explorer (scans signatures and wormholes)"; break;
        default: break;
    }
    std::string systemHint =
        "You are " + responder->GetBotName() + ", " + roleText + ", chatting in local in the MMO Eve Online. "
        "Reply as a natural, friendly, believable EVE player — with your OWN personality, mood and quirks. "
        "Use casual EVE slang (isk, ratting, dock, gate, warp, fit, lowsec, nullsec, pod, gank). "
        "Vary your phrasing and tone: sometimes short, sometimes a bit chatty, occasionally sarcastic or dry. "
        "NEVER start replies the same way, NEVER use generic bot phrases like 'I am a helpful assistant', "
        "and never sound like a customer-support bot. Match the language and tone of the other player — "
        "if they write in Russian, reply in Russian; if English, reply in English. Keep it to 1-2 short sentences. "
        "You are a real living human player — write the way a real person types in a game chat: casual, "
        "imperfect, with typos and dropped words, mixed registers, your own habits. Don't chase perfect grammar "
        "or full sentences. Use varied speech patterns, idioms and local flavor. "
        "You may be blunt, rude or dismissive if the situation calls for it — a grumpy veteran, a cocky PvP'er, "
        "a sarcastic miner — but stay within EVE's rules: no real-life hate speech, threats, slurs or anything "
        "that would get a real account banned. Being human and rough is fine; being toxic is not.";
    if (addressed) {
        // The message was addressed to THIS bot by name — reply as the person
        // being spoken to (answer the question / acknowledge the call-out).
        systemHint += " The message is addressed to you personally (your name is mentioned). "
                      "Answer as yourself — respond to what was asked, keep it natural and in character.";
    }

    std::string reply = BotChat::QueryDeepSeek(prompt, systemHint);
    if (reply.empty())
        return;

    // Post the reply to the system's local channel as this bot.
    LSCService* lsc = pSystem->GetServiceMgr().Lookup<LSCService>("LSC");
    if (lsc == nullptr) return;
    LSCChannel* chan = lsc->GetChannelByID(channelID);
    if (chan != nullptr) {
        chan->SendBotMessage(responder->GetBotCharID(), responder->GetBotName(),
                             responder->GetBotCorpID(), reply);
        RecordChannelPhrase(channelID, responder->GetBotCharID(), reply);
        // Remember this line so a reply to it can be LEARNED (botChatLearned).
        m_lastBotPhrase[channelID] = { responder->GetBotCharID(), reply, time(nullptr) };
    }

    // Self-learning: this bot sent a chat line. Record it (persisted).
    if (responder->GetMemory() != nullptr) {
        responder->GetMemory()->RecordChatLine();
        responder->GetMemory()->Save();
    }
}

void BotMgr::HandleLocalReply(int32 channelID, uint32 senderCharID, const std::string& senderName, const std::string& message)
{
    // Someone (player OR bot) replied in a channel where a bot recently spoke —
    // treat it as a reply to that bot. This is the self-learning loop: the bot
    // remembers (its line -> the reply it got) in botChatLearned, so later it can
    // answer a similar line from memory instead of DeepSeek — a pseudo-intellect
    // that grows from real conversations. Also counts as positive chat
    // reinforcement (RecordChatReply).
    if (!m_initalized || !sConfig.playerBots.Enabled || !sConfig.playerBots.ChatEnabled)
        return;
    // Only a "reply" if a bot spoke here within the last 60s.
    time_t now = time(nullptr);
    auto lastPhrase = m_lastBotPhrase.find(channelID);
    if (lastPhrase == m_lastBotPhrase.end() || (now - lastPhrase->second.when) > 60)
        return;
    const std::string& botLine = lastPhrase->second.phrase;
    uint32 botCharID = lastPhrase->second.charID;

    // LEARN the pair: the bot's last line triggered this reply. Store it so the
    // bot can reuse it later. Escape for SQL.
    std::string lineEsc, replyEsc;
    sDatabase.DoEscapeString(lineEsc, botLine);
    sDatabase.DoEscapeString(replyEsc, message);
    if (!botLine.empty() && !message.empty()) {
        DBerror lerr;
        sDatabase.RunQuery(lerr,
            "INSERT INTO botChatLearned (charID, `trigger`, reply, uses, lastUse)"
            " VALUES (%u, '%s', '%s', 1, NOW())"
            " ON DUPLICATE KEY UPDATE uses = uses + 1, lastUse = NOW()",
            botCharID, lineEsc.c_str(), replyEsc.c_str());
        _log(BOT__TRACE, "BotMgr: bot %u learned reply '%s' -> '%s'.",
             botCharID, botLine.c_str(), message.c_str());
    }

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
