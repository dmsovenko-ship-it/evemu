#include "eve-server.h"
#include "npc/BotMgr.h"
#include "npc/BotMemory.h"
#include "npc/PlayerBot.h"
#include "npc/NPCAI.h"
#include "npc/NPC.h"
#include "npc/BotChat.h"
#include "EntityList.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "corporation/CorporationDB.h"
#include "character/CharacterDB.h"
#include "chat/LSCService.h"
#include "chat/LSCChannel.h"
#include "services/ServiceManager.h"
#include "account/AccountService.h"
#include "market/MarketMgr.h"
#include "market/MarketDB.h"
#include "ship/Ship.h"
#include "EVE_Effects.h"
#include "pos/Tower.h"
#include "pos/Array.h"
#include "pos/Module.h"
#include "pos/Structure.h"
#include "pos/Battery.h"
#include "pos/Weapon.h"
#include "planet/CustomsOffice.h"
#include "tables/invGroups.h"
#include "inventory/AttributeEnum.h"
#include "TelegramBot.h"
#include "character/Character.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <unistd.h>

static void SecurityAuditTick();
static void DailyKillDigestTick();
static void ProcessBotTrainingBatch();
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

void BotMgr::CleanupOrphanedSpaceItems()
{
    DBerror err;
    uint32 affected = 0;

    // 0) Reset stale chelobot session rows first: a docked/reaped bot keeps
    //    online=1 plus a shipID pointing at a ship that was deleted together
    //    with it, which would make the sweeps below keep its crash leftovers.
    //    Bots spawn on demand and rewrite these fields, so a blank slate is
    //    correct (portal shows the offline hull from botMemory.shipTypeID).
    if (!sDatabase.RunQuery(err,
        "UPDATE chrCharacters c JOIN botMemory bm ON bm.charID = c.characterID "
        "SET c.online = 0, c.stationID = 0, c.shipID = 0, c.solarSystemID = 0"))
        _log(BOT__ERROR, "BotMgr: bot session reset failed: %s", err.GetError());

    // 1) Orphan ships: corp-owned hulls in space that no character lists as
    //    its active ship (chelobot hulls after a crash, transient NPC spawns).
    //    Their fitted modules and loaded charges (children, grandchildren) and
    //    all entity_attributes are removed with them. System range 3000xxxx
    //    (k-space) .. 31xxxxxx (w-space) — stations/planets/moons are outside.
    if (!sDatabase.RunQuery(err, affected,
        "DELETE e, ea, ec, eac, eg, eag FROM entity e "
        "LEFT JOIN entity_attributes ea ON ea.itemID = e.itemID "
        "LEFT JOIN entity ec ON ec.locationID = e.itemID "
        "LEFT JOIN entity_attributes eac ON eac.itemID = ec.itemID "
        "LEFT JOIN entity eg ON eg.locationID = ec.itemID "
        "LEFT JOIN entity_attributes eag ON eag.itemID = eg.itemID "
        "JOIN invTypes t ON t.typeID = e.typeID "
        "JOIN invGroups g ON g.groupID = t.groupID "
        "WHERE g.categoryID = 6 AND e.flag = 0 "
        "AND e.locationID >= 30000000 AND e.locationID < 40000000 "
        "AND e.ownerID IN (SELECT corporationID FROM crpCorporation) "
        "AND e.itemID NOT IN (SELECT shipID FROM chrCharacters WHERE shipID > 0)"))
        _log(BOT__ERROR, "BotMgr: orphan ship cleanup failed: %s", err.GetError());
    else if (affected > 0)
        sLog.White("      BotMgr", "Space cleanup: removed %u leftover in-space ship rows (bot hulls/NPC spawns).", affected);

    // 2) Orphan drones: owner is not a pilot actively flying in that system —
    //    docked or logged-off pilots never have legal drones in space, and
    //    NPC drones (owner = faction corp, no character row) are leftovers.
    if (!sDatabase.RunQuery(err, affected,
        "DELETE e, ea FROM entity e "
        "LEFT JOIN entity_attributes ea ON ea.itemID = e.itemID "
        "LEFT JOIN chrCharacters c ON c.characterID = e.ownerID "
        "LEFT JOIN entity sh ON sh.itemID = c.shipID "
        "JOIN invTypes t ON t.typeID = e.typeID "
        "JOIN invGroups g ON g.groupID = t.groupID "
        "WHERE g.categoryID = 18 AND e.flag = 0 "
        "AND e.locationID >= 30000000 AND e.locationID < 40000000 "
        "AND (c.characterID IS NULL OR c.online = 0 OR c.stationID <> 0 "
        "OR c.solarSystemID <> e.locationID OR sh.itemID IS NULL)"))
        _log(BOT__ERROR, "BotMgr: orphan drone cleanup failed: %s", err.GetError());
    else if (affected > 0)
        sLog.White("      BotMgr", "Space cleanup: removed %u orphaned drones (pilot docked/offline).", affected);

    // 3) the stable-pilot-pool trim (see TrimPilotPool below for details).
    TrimPilotPool();
}

// Boot-time pilot-pool trim: if a previous over-spawn (or repeated cleanup+
// spawn cycles) pushed the persistent pool past MaxTotalPilots, delete the
// NEWEST characters beyond the cap together with their memory/portrait maps,
// private mail, killmails and owned items. The oldest ~N capped pilots stay —
// the spawner reuses them with their saved professions instead of rolling
// fresh legends, so the population is one stable capped set (user rule).
void BotMgr::TrimPilotPool()
{
    DBerror err;
    uint32 cap = sConfig.playerBots.MaxTotalPilots;
    if (cap == 0)
        return;
    DBQueryResult cres;
    uint32 poolCount = 0;
    sDatabase.RunQuery(cres,
        "SELECT COUNT(*) FROM chrCharacters WHERE accountID = 0 AND characterName != ''");
    {
        DBResultRow crow;
        if (cres.GetRow(crow))
            poolCount = crow.GetUInt(0);
    }
    uint32 overflow = (poolCount > cap) ? poolCount - cap : 0;
    if (overflow == 0)
        return;

    DBQueryResult ores;
    if (!sDatabase.RunQuery(ores,
        "SELECT characterID FROM chrCharacters WHERE accountID = 0 AND characterName != ''"
        " ORDER BY characterID DESC LIMIT %u", overflow)) {
        _log(BOT__ERROR, "BotMgr: pool trim query failed.");
        return;
    }
    std::string ids;
    DBResultRow orow;
    bool any = false;
    while (ores.GetRow(orow)) {
        if (any) ids += ",";
        ids += std::to_string(orow.GetUInt(0));
        any = true;
    }
    if (!any)
        return;

    sDatabase.RunQuery(err,
        "DELETE k FROM chrKillTable k WHERE k.victimCharacterID IN (%s) OR k.finalCharacterID IN (%s)",
        ids.c_str(), ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE p FROM botPortraits p WHERE p.serverCharID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE b FROM botMemory b WHERE b.charID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE h FROM chrSkillHistory h WHERE h.characterID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE e FROM chrEmployment e WHERE e.characterID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE st FROM mailStatus st WHERE st.characterID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE m FROM mailMessage m WHERE m.senderID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE e, ea FROM entity e LEFT JOIN entity_attributes ea ON ea.itemID = e.itemID"
        " WHERE e.ownerID IN (%s)", ids.c_str());
    sDatabase.RunQuery(err,
        "DELETE FROM chrCharacters WHERE characterID IN (%s)", ids.c_str());

    sLog.White("      BotMgr", "Pilot pool trim: removed %u newest overflow pilots (pool %u -> %u).",
               overflow, poolCount, cap);
}

int BotMgr::Initialize()
{
    m_initalized = true;
    // NOTE: the DB may not be connected yet when this runs (eve-server.cpp
    // connects later), so the space cleanup is NOT done here — main() calls
    // CleanupOrphanedSpaceItems() right after sDatabase.Initialize().
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
    static time_t sLastReap = 0;
    time_t reapClock = time(nullptr);
    bool reapNow = (sLastReap == 0 || reapClock - sLastReap >= 30);
    if (reapNow) sLastReap = reapClock;

    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr)
            continue;
        if (pSystem->PlayerCount() < 1) {
            // No real player here any more — reap the simulated population so
            // bots follow the players (they were staying in the old system
            // forever because ReapBots() was never called). Throttled.
            if (reapNow)
                ReapBots(pSystem);
            continue;
        }

        PopulateSystem(pSystem);
    }

    // Occasionally let a bot travel to a neighbouring system (through a gate),
    // so the "population" moves around like a live server.
    ProcessTravel();

    // Manage docked bots: undock some each tick, dock others.
    ProcessDocking();

    // Docked traders work the market from their station.
    ProcessDockedEconomy();

    // POS guards assist the tower operator's target (manual gunnery focus fire).
    ProcessPosGuards();

    // Bots occasionally chatter among themselves in local (rare).
    ProcessBotSmalltalk();

    // Drain queued bot chat replies (one per tic) — drives bot<-bot conversations
    // without recursing the stack.
    ProcessBotReplies();

    // Courier bots pick up unaccepted player courier contracts.
    ProcessPlayerContracts();

    // Complete courier hauls that reached their destination but could not dock.
    ProcessHaulDeliveries();

    // Refresh the portal's "online" figure (active + docked chelobots) on the
    // game thread so the API thread can read it race-free.
    RefreshOnlineCount();

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

    // Periodic admin security audit (RMT flows / multiboxing IPs) → admin TG.
    SecurityAuditTick();

    // Daily top-kills digest → public (player) TG group.
    DailyKillDigestTick();

    // Player-like offline skill training for simulated pilots.
    ProcessBotTrainingBatch();
}

// Periodic admin security audit. Scans the last 24h of market fills for
// unusually large human↔human ISK flows (RMT) and the login history for several
// accounts sharing one IP (multiboxing), then notifies the closed admin Telegram
// group. Runs at most every 10 minutes; per-finding notifications are
// rate-limited to ~once per 6 hours so a persistent pattern doesn't spam.
static time_t sLastSecurityScan = 0;
static std::map<std::string, time_t> sSecuritySent;

static void SecurityAuditTick()
{
    if (!sConfig.telegram.AdminEnabled)
        return;
    time_t now = time(nullptr);
    if (now - sLastSecurityScan < sConfig.security.AuditIntervalSec)
        return;
    sLastSecurityScan = now;

    auto dedupe = [&](const std::string& key) -> bool {
        auto it = sSecuritySent.find(key);
        if (it != sSecuritySent.end()
            && now - it->second < sConfig.security.AlertCooldownSec)
            return false;
        sSecuritySent[key] = now;
        return true;
    };

    std::string body;
    int found = 0;

    // 1) Big human↔human ISK flows (mktTransactions, sell side only, 24h).
    //    Human = a character whose account exists in `account` (chelobots have
    //    accountID 0 and never trip this).
    {
        int64 since = GetFileTimeNow()
                    - (int64)sConfig.security.FlowWindowHours * 3600LL * 10000000LL;
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT t.clientID AS sellerID, sc.characterName AS sellerName,"
            "       t.characterID AS buyerID, bc.characterName AS buyerName,"
            "       COUNT(*) AS trades, SUM(t.price * t.quantity) AS isk"
            " FROM mktTransactions t"
            " JOIN chrCharacters sc ON sc.characterID = t.clientID"
            " JOIN chrCharacters bc ON bc.characterID = t.characterID"
            " WHERE t.transactionType = 0 AND t.transactionDate >= %lli"
            "   AND t.clientID IN (SELECT characterID FROM chrCharacters"
            "                       WHERE accountID IN (SELECT accountID FROM account))"
            "   AND t.characterID IN (SELECT characterID FROM chrCharacters"
            "                          WHERE accountID IN (SELECT accountID FROM account))"
            "   AND t.clientID <> t.characterID"
            "   AND NOT EXISTS (SELECT 1 FROM accountTransfers at"
            "                   WHERE at.sellerAccountID = sc.accountID"
            "                     AND at.buyerAccountID = bc.accountID)"
            " GROUP BY t.clientID, t.characterID"
            " HAVING SUM(t.price * t.quantity) >= %llu"
            " ORDER BY isk DESC LIMIT 6",
            (long long)since, (unsigned long long)sConfig.security.FlowThresholdISK))
        {
            DBResultRow row;
            while (res.GetRow(row)) {
                std::string sig = "flow:" + std::to_string(row.GetUInt(0)) + ">"
                                + std::to_string(row.GetUInt(2));
                if (!dedupe(sig))
                    continue;
                body += "\n- FLOW " + std::to_string((int64)row.GetDouble(5))
                      + " ISK over " + std::to_string(row.GetUInt(4)) + " trades: "
                      + row.GetText(1) + " -> " + row.GetText(3);
                ++found;
            }
        }
    }

    // 2) Accounts sharing one IP (last 14 days) → multiboxing hint.
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT h.ip, COUNT(DISTINCT h.accountID) AS cnt,"
            "       GROUP_CONCAT(DISTINCT a.accountName SEPARATOR ', ') AS names"
            " FROM accountLoginHistory h"
            " JOIN account a ON a.accountID = h.accountID"
            " WHERE h.loginTime >= NOW() - INTERVAL %u DAY"
            " GROUP BY h.ip"
            " HAVING cnt >= %u"
            " ORDER BY cnt DESC LIMIT 8",
            sConfig.security.IPWindowDays, sConfig.security.MinAccountsSameIP))
        {
            DBResultRow row;
            while (res.GetRow(row)) {
                std::string sig = "ip:" + std::string(row.GetText(0));
                if (!dedupe(sig))
                    continue;
                body += "\n- IP " + std::string(row.GetText(0)) + " -> "
                      + std::to_string(row.GetUInt(1)) + " accounts: "
                      + row.GetText(2);
                ++found;
            }
        }
    }

    if (found > 0)
        TelegramBot::NotifyAdmin("🛡 Security: " + std::to_string(found)
            + " флаг(ов)" + body);
}

// Daily top-kills digest → public (player) Telegram group. Fires at most once
// per day (24h from the previous run).  Rows are enriched: local time of the
// kill, victim (corp + ship class), system + region, the final-blow ship class
// and who landed it, and damage — so the message reads like a mini killboard.
static time_t sLastKillDigest = 0;

std::string HumanizeIsk(double v) {
    char buf[64];
    if (v >= 1000000000.0)      snprintf(buf, sizeof(buf), "%.2fb", v / 1000000000.0);
    else if (v >= 1000000.0)    snprintf(buf, sizeof(buf), "%.2fm", v / 1000000.0);
    else if (v >= 1000.0)       snprintf(buf, sizeof(buf), "%.1fk", v / 1000.0);
    else                        snprintf(buf, sizeof(buf), "%.0f", v);
    return buf;
}

// Builds the enriched top-kills block (shared by the daily digest and the
// /topkills Telegram command).  `sinceSql` is a SQL boolean restricting the
// window (e.g. the last 24h) — pass "1" for an overall leaderboard.
std::string BuildKillDigestText(int limit, const std::string& sinceSql)
{
    char lim[16];
    snprintf(lim, sizeof(lim), "%d", limit);
    std::string q =
        "SELECT k.victimCharacterID, vc.characterName,"
        "       vcc.corporationName,"
        "       iv.typeName, igv.groupName,"
        "       k.solarSystemID, ss.solarSystemName, rg.regionName,"
        "       fc.characterName, if_.typeName, igf.groupName,"
        "       k.victimDamageTaken,"
        "       DATE_FORMAT(FROM_UNIXTIME((k.killTime - 116444736000000000) / 10000000), '%d.%m %H:%i') AS kt"
        " FROM chrKillTable k"
        " LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID"
        " LEFT JOIN crpCorporation vcc ON vcc.corporationID = k.victimCorporationID"
        " LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID"
        " LEFT JOIN invGroups igv ON igv.groupID = iv.groupID"
        " LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID"
        " LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID"
        " LEFT JOIN invGroups igf ON igf.groupID = if_.groupID"
        " LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID"
        " LEFT JOIN mapRegions rg ON rg.regionID = ss.regionID"
        " WHERE " + sinceSql +
        " ORDER BY k.victimDamageTaken DESC LIMIT " + lim;
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, q.c_str()))
        return "";

    std::string body;
    int count = 0;
    DBResultRow row;
    while (res.GetRow(row)) {
        const char* victim = row.GetText(1) ? row.GetText(1) : "unknown";
        const char* vcorp  = row.GetText(2);
        const char* ship   = row.GetText(3) ? row.GetText(3) : "";
        const char* grp    = row.GetText(4);
        const char* sys    = row.GetText(6) ? row.GetText(6) : "";
        const char* region = row.GetText(7);
        const char* killer = row.GetText(8);
        const char* kship  = row.GetText(9);
        const char* kgrp   = row.GetText(10);
        const char* ktime  = row.GetText(12) ? row.GetText(12) : "";

        std::string line = "\n";
        if (*ktime) { line += "🕐 " + std::string(ktime) + " · "; }
        line += victim;
        if (vcorp) line += " <" + std::string(vcorp) + ">";
        if (*ship) line += " — " + std::string(ship);
        if (grp)   line += " [" + std::string(grp) + "]";
        if (*sys)  line += "\n      📍 " + std::string(sys);
        if (region) line += " (" + std::string(region) + ")";
        // final blow: pilot name if a character did it, else the NPC ship name
        line += "\n      ⚔ ";
        if (killer) line += std::string(killer);
        else if (kship) line += std::string(kship);
        else line += "NPC";
        if (kship && killer) line += " на " + std::string(kship);
        if (kgrp) line += " [" + std::string(kgrp) + "]";
        line += " · dmg " + HumanizeIsk(row.GetUInt(11));
        body += line;
        ++count;
    }
    if (count == 0)
        return "";
    return "📊 Top-" + std::to_string(count) + " киллов за сутки:" + body;
}

static void DailyKillDigestTick()
{
    if (!sConfig.telegram.PlayerEnabled)
        return;
    time_t now = time(nullptr);
    if (sLastKillDigest != 0 && now - sLastKillDigest < 86400)
        return;
    sLastKillDigest = now;

    std::string digest = BuildKillDigestText(5,
        "(k.killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 DAY))");
    if (!digest.empty()) {
        TelegramBot::NotifyPlayer(digest);
        TelegramBot::NotifyAdmin(digest);
    }
}

// Player-like skill training for simulated pilots. Runs every 5 minutes over a
// batch of online chelobots. Attributes are base-by-bloodline x multiplier; the
// current skill (lowest typeID still below V) accumulates real SP at
// primary+secondary/2 points/minute; on completion the skill level + points and
// the character's total skillPoints are bumped. DB-only (no Client session).
static time_t sLastBotTraining = 0;

static void ProcessBotTrainingBatch()
{
    auto& cfg = sConfig.playerBots;
    if (!cfg.Enabled || !cfg.TrainingEnabled)
        return;
    time_t now = time(nullptr);
    if (sLastBotTraining != 0 && now - sLastBotTraining < 300)
        return;
    sLastBotTraining = now;

    // ---- roster profession rebalance (user rule): a biologist/trainee keeps
    // its own job; ONLY when a profession runs short roster-wide do a few
    // pilots switch to the missing one WITHOUT losing progress — all learned
    // skills stay on the character, only botMemory.profession (and, on the
    // next respawn, the hull/behaviour) changes. Runs at most once per 30 min.
    static time_t sLastProfBalance = 0;
    if (sLastProfBalance == 0 || now - sLastProfBalance >= 1800) {
        sLastProfBalance = now;
        DBQueryResult pc;
        if (sDatabase.RunQuery(pc,
            "SELECT b.profession, COUNT(*) FROM botMemory b"
            " JOIN chrCharacters c ON c.characterID = b.charID AND c.accountID = 0"
            " GROUP BY b.profession"))
        {
            uint32 cnt[9] = {0};   // Hunter..Industrialist (0..8)
            DBResultRow prow;
            while (pc.GetRow(prow)) {
                uint8 p = prow.GetUInt(0);
                if (p < 9) cnt[p] = prow.GetUInt(1);
            }
            uint8 least = 0xFF, most = 0;
            uint32 total = 0;
            for (int i = 0; i < 9; ++i) {
                total += cnt[i];
                if (least == 0xFF || cnt[i] < cnt[least]) least = (uint8)i;
                if (cnt[i] > cnt[most]) most = (uint8)i;
            }
            uint32 minPer = std::max<uint32>(3, total / 9 / 4);   // quarter of the even share
            if (least != 0xFF && least != most && cnt[least] < minPer) {
                // A brand-new profession (e.g. Industrialist) starts empty — seed a
                // real share of the roster in one pass so it is actually present.
                uint32 toMove;
                if (cnt[least] == 0)
                    toMove = std::max<uint32>(5, std::min<uint32>(cnt[most] / 8, 80));
                else
                    toMove = std::max<uint32>(1, std::min<uint32>((cnt[most] - cnt[least]) / 3, 5));
                // movers: LOWEST-skillPoints pilots of the biggest profession
                // (newcomers switch careers easiest; veterans keep their job).
                DBQueryResult mres;
                if (sDatabase.RunQuery(mres,
                    "SELECT b.charID FROM botMemory b"
                    " JOIN chrCharacters c ON c.characterID = b.charID AND c.accountID = 0"
                    " WHERE b.profession = %u"
                    " ORDER BY COALESCE(c.skillPoints, 0) ASC LIMIT %u", most, toMove))
                {
                    DBerror merr;
                    DBResultRow mrow;
                    uint32 moved = 0;
                    while (mres.GetRow(mrow)) {
                        sDatabase.RunQuery(merr,
                            "UPDATE botMemory SET profession = %u WHERE charID = %u",
                            least, mrow.GetUInt(0));
                        ++moved;
                    }
                    if (moved > 0)
                        sLog.White("      BotMgr", "Roster rebalance: %u pilot(s) switched %u -> %u"
                            " (short profession; skills kept).", moved, (unsigned)most, (unsigned)least);
                }
            }
        }
    }

    const int64 nowFt = GetFileTimeNow();
    const int64 ftSec = 10000000LL;
    float mult = cfg.AttrMultiplier > 0.0f ? cfg.AttrMultiplier
                                          : (float)sConfig.character.statMultiplier;
    if (mult <= 0.0f) mult = 1.0f;

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT c.characterID, c.raceID, b.profession FROM chrCharacters c"
        " JOIN botMemory b ON b.charID = c.characterID"
        " WHERE c.accountID = 0 AND c.online = 1 LIMIT 300"))
        return;

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 charID = row.GetUInt(0);
        uint32 race   = row.GetUInt(1);
        uint8  prof   = (uint8)row.GetUInt(2);

        // --- attributes: race-based base x multiplier with per-pilot variation
        // (deterministic — no dependence on bloodline tables that may be empty).
        int16 baseA[5] = { 19, 19, 19, 19, 19 };
        switch (race) {
            case 1: baseA[0]=20;baseA[1]=20;baseA[2]=21;baseA[3]=21;baseA[4]=18; break; // Amarr
            case 2: baseA[0]=21;baseA[1]=21;baseA[2]=20;baseA[3]=19;baseA[4]=19; break; // Caldari
            case 4: baseA[0]=19;baseA[1]=19;baseA[2]=21;baseA[3]=21;baseA[4]=20; break; // Minmatar
            case 8: baseA[0]=20;baseA[1]=19;baseA[2]=21;baseA[3]=20;baseA[4]=20; break; // Gallente
        }
        int seed = (int)(charID % 7) - 3;   // -3..+3 individual variance
        int16 aInt  = (int16)std::lround((baseA[0] + seed) * mult);
        int16 aMem  = (int16)std::lround((baseA[1] + seed) * mult);
        int16 aPer  = (int16)std::lround((baseA[2] + seed) * mult);
        int16 aWill = (int16)std::lround((baseA[3] + seed) * mult);
        int16 aCha  = (int16)std::lround((baseA[4] + seed) * mult);

        // --- training row (create if missing) ---
        DBQueryResult tr;
        uint32 tSkill = 0, tNext = 1;
        double tProg = 0;
        int64 tLast = 0;
        bool haveRow = false;
        if (sDatabase.RunQuery(tr, "SELECT skillTypeID, nextLevel, spProgress, lastTrain"
                                   " FROM botTraining WHERE charID = %u", charID)) {
            DBResultRow r;
            if (tr.GetRow(r)) {
                tSkill = r.GetUInt(0); tNext = r.GetUInt(1);
                tProg  = r.GetDouble(2); tLast = r.GetInt64(3);
                haveRow = true;
            }
        }
        if (!haveRow) {
            DBerror e;
            sDatabase.RunQuery(e,
                "INSERT IGNORE INTO botTraining (charID, attrInt, attrMem, attrPer, attrWill, attrCha, lastTrain)"
                " VALUES (%u, %d, %d, %d, %d, %d, %lli)",
                charID, aInt, aMem, aPer, aWill, aCha, (long long)nowFt);
            tLast = nowFt;   // first tick starts now (no retro SP)
        }
        // Always refresh attributes in the row (fixes any rows created with 0).
        {
            DBerror e;
            sDatabase.RunQuery(e,
                "UPDATE botTraining SET attrInt = %d, attrMem = %d, attrPer = %d,"
                " attrWill = %d, attrCha = %d WHERE charID = %u",
                aInt, aMem, aPer, aWill, aCha, charID);
        }

        int64 since = nowFt - tLast;
        if (since < ftSec) {
            // update lastTrain so later ticks are still accurate
            DBerror e;
            sDatabase.RunQuery(e, "UPDATE botTraining SET lastTrain = %lli WHERE charID = %u",
                               (long long)nowFt, charID);
            continue;
        }

        // --- pick current training skill: keep stored if still < V; otherwise
        // the bot trains the skills of ITS OWN profession (user rule) ---
        uint32 curType = 0, curLevel = 5;
        {
            DBQueryResult sres;
            if (sDatabase.RunQuery(sres,
                "SELECT s.typeID, COALESCE((SELECT a.valueInt FROM entity_attributes a"
                "   WHERE a.itemID = s.itemID AND a.attributeID = 280), 0) AS lvl"
                " FROM entity s WHERE s.ownerID = %u AND s.flag = 7 AND s.typeID > 0"
                " ORDER BY s.typeID", charID))
            {
                DBResultRow srow;
                bool storedOk = false;
                while (sres.GetRow(srow)) {
                    uint32 typeID = srow.GetUInt(0);
                    uint32 lvl = srow.GetUInt(1);
                    if (curType == 0) { curType = typeID; curLevel = lvl; }   // lowest first
                    if (tSkill != 0 && typeID == tSkill) storedOk = (lvl < 5);
                }
                if (storedOk) { curType = tSkill; curLevel = 0; }   // refetch below
            }
        }
        if (curType != 0 && curLevel < 5) {
            // Profession-first preference (user rule): the pilot levels the
            // skillbook of its own job while something below V remains there.
            static const char* profPool[8][10] = {
                /* Hunter   */ { "Gunnery", "Shield ", "Armor ", "Navigation", "Afterburner",
                                 "Warp Drive%", "Propulsion Jamming", "Motion Prediction",
                                 "Trajectory Analysis", "Evasive Maneuvering" },
                /* RatHunter*/ { "Drones", "Drone Interfacing", "Drone Navigation",
                                 "Drone Durability", "Gunnery", "Repair Systems",
                                 "Repair Commissioning", "Shield ", "Armor ", "Leadership" },
                /* Miner    */ { "Mining", "Astrogeology", "Ice Harvesting", "Deep Core Mining",
                                 "Mining Upgrades", "Mining Foreman", "Mining Barge", "Exhumer",
                                 "Refining", "Industrial Command Ships" },
                /* Trader   */ { "Trade", "Retail", "Marketing", "Accounting", "Broker Relations",
                                 "Visibility", "Procurement", "Daytrading", "Margin Trading",
                                 "Negotiation" },
                /* Courier  */ { "Navigation", "Warp Drive Operation", "Evasive Maneuvering",
                                 "Afterburner", "Industrial", "Freight Containers",
                                 "Jump Drive%", "Hull Upgrades", "Energy Grid%", "Mechanics" },
                /* Hacker   */ { "Hacking", "Archeology", "Data Analysis", "CPU Management",
                                 "Survey", "Codebreaker", "Electronics", "Signature Analysis",
                                 "Target Management", "Long Range Targeting" },
                /* Explorer */ { "Astrometric", "Astrometry", "Cloaking", "Hacking", "Archeology",
                                 "Survey", "Long Range Targeting", "Signature Analysis",
                                 "Cruiser", "Evasive Maneuvering" },
                /* Missioner*/ { "Social", "Connections", "Diplomacy", "Gunnery", "Drones",
                                 "Repair Systems", "Shield ", "Armor ", "Leadership",
                                 "Targeting" },
            };
            uint8 pi = prof < 8 ? prof : 0;
            std::string profWhere;
            for (int k = 0; k < 10; ++k) {
                if (profPool[pi][k] == nullptr || profPool[pi][k][0] == 0) break;
                std::string pat = profPool[pi][k];
                if (pat.find('%') == std::string::npos) pat += "%";
                if (!profWhere.empty()) profWhere += " OR ";
                profWhere += "t.typeName LIKE '";
                profWhere += pat;
                profWhere += "'";
            }
            if (!profWhere.empty()) {
                DBQueryResult psel;
                if (sDatabase.RunQuery(psel,
                    "SELECT s.typeID, COALESCE((SELECT a.valueInt FROM entity_attributes a"
                    "   WHERE a.itemID = s.itemID AND a.attributeID = 280), 0) AS lvl"
                    " FROM entity s JOIN invTypes t ON t.typeID = s.typeID"
                    " WHERE s.ownerID = %u AND s.flag = 7 AND s.typeID > 0"
                    "   AND (%s)"
                    " ORDER BY s.typeID LIMIT 1", charID, profWhere.c_str()))
                {
                    DBResultRow prow;
                    if (psel.GetRow(prow) && prow.GetUInt(1) < 5) {
                        curType = prow.GetUInt(0);
                        curLevel = 0;   // refetch below
                    }
                }
            }
        }
        if (curType == 0 || curLevel >= 5) {
            // nothing left to train (all V) — mark idle
            DBerror e;
            sDatabase.RunQuery(e, "UPDATE botTraining SET skillTypeID = 0, nextLevel = 1,"
                                  " spProgress = 0, lastTrain = %lli WHERE charID = %u",
                               (long long)nowFt, charID);
            continue;
        }
        if (curLevel == 0) {
            // kept stored skill — load its real level
            DBQueryResult lr;
            if (sDatabase.RunQuery(lr, "SELECT valueInt FROM entity_attributes a"
                                       " JOIN entity e ON e.itemID = a.itemID"
                                       " WHERE e.ownerID = %u AND e.typeID = %u AND e.flag = 7"
                                       " AND a.attributeID = 280 LIMIT 1", charID, curType)) {
                DBResultRow lrow;
                if (lr.GetRow(lrow)) curLevel = lrow.GetUInt(0); else curLevel = 5;
            } else curLevel = 5;
        }

        uint32 next = curLevel + 1;
        if (next > 5) next = 5;

        // --- rank (time constant) + primary/secondary attribute ids for the skill ---
        float rank = 1.0f;
        uint32 priId = 165, secId = 166;   // defaults: Int / Mem
        {
            DBQueryResult tq;
            if (sDatabase.RunQuery(tq, "SELECT valueFloat FROM dgmTypeAttributes"
                                       " WHERE typeID = %u AND attributeID = 275 LIMIT 1", curType)) {
                DBResultRow trow;
                if (tq.GetRow(trow)) rank = trow.GetFloat(0);
            }
            if (sDatabase.RunQuery(tq, "SELECT valueFloat FROM dgmTypeAttributes"
                                       " WHERE typeID = %u AND attributeID = 180 LIMIT 1", curType)) {
                DBResultRow trow;
                if (tq.GetRow(trow)) priId = (uint32)trow.GetFloat(0);
            }
            if (sDatabase.RunQuery(tq, "SELECT valueFloat FROM dgmTypeAttributes"
                                       " WHERE typeID = %u AND attributeID = 181 LIMIT 1", curType)) {
                DBResultRow trow;
                if (tq.GetRow(trow)) secId = (uint32)trow.GetFloat(0);
            }
        }
        int attrFor[170] = {0};
        attrFor[164] = aCha; attrFor[165] = aInt; attrFor[166] = aMem;
        attrFor[167] = aPer; attrFor[168] = aWill;
        int primaryV = attrFor[priId % 170] != 0 ? attrFor[priId % 170] : aInt;
        int secondaryV = attrFor[secId % 170] != 0 ? attrFor[secId % 170] : aMem;
        double spm = (double)EvEMath::Skill::PointsPerMinute((uint8)primaryV, (uint8)secondaryV);
        if (spm <= 0.0) spm = 27.0;

        double minutes = (double)(since / ftSec) / 60.0;
        if (minutes < 1.0 / 3600.0) {   // <1s
            DBerror e;
            sDatabase.RunQuery(e, "UPDATE botTraining SET lastTrain = %lli WHERE charID = %u",
                               (long long)nowFt, charID);
            continue;
        }
        tProg += minutes * spm;
        // Visible, continuous SP growth: add what was earned this tick to the
        // character's total (like EVE counts partial SP toward the level).
        {
            int64 tickEarn = (int64)(minutes * spm);
            if (tickEarn > 0) {
                DBerror e;
                sDatabase.RunQuery(e,
                    "UPDATE chrCharacters SET skillPoints = skillPoints + %lld WHERE characterID = %u",
                    (long long)tickEarn, charID);
            }
        }

        uint32 needBase = EvEMath::Skill::PointsAtLevel(curLevel, rank);
        uint32 needNext = EvEMath::Skill::PointsAtLevel(next, rank);
        uint32 need = needNext > needBase ? needNext - needBase : 0;

        bool leveled = (need > 0 && tProg >= need);
        if (leveled) {
            uint32 newPoints = needNext;
            // update skill item level/points
            DBerror e;
            sDatabase.RunQuery(e,
                "UPDATE entity_attributes a JOIN entity e ON e.itemID = a.itemID"
                " SET a.valueInt = %u WHERE e.ownerID = %u AND e.typeID = %u"
                "   AND e.flag = 7 AND a.attributeID = 280", next, charID, curType);
            sDatabase.RunQuery(e,
                "UPDATE entity_attributes a JOIN entity e ON e.itemID = a.itemID"
                " SET a.valueInt = %u WHERE e.ownerID = %u AND e.typeID = %u"
                "   AND e.flag = 7 AND a.attributeID = 276", newPoints, charID, curType);
            // total SP already gained incrementally via tickEarn
            tProg = 0.0;
        }

        DBerror e;
        sDatabase.RunQuery(e,
            "UPDATE botTraining SET skillTypeID = %u, nextLevel = %u, spProgress = %f,"
            " lastTrain = %lli WHERE charID = %u",
            leveled && next >= 5 ? 0u : curType, leveled ? 1u : next, tProg,
            (long long)nowFt, charID);
    }
}

void BotMgr::RefreshOnlineCount()
{
    uint32 active = 0;
    uint32 docked = 0;
    if (sConfig.playerBots.Enabled) {
        for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
            if (pSystem == nullptr)
                continue;
            for (auto& [id, se] : pSystem->GetEntities()) {
                if (se == nullptr || se->GetNPCSE() == nullptr)
                    continue;
                if (dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
                    ++active;
            }
        }
        for (auto& [sysID, bots] : m_docked)
            docked += (uint32)bots.size();
    }
    m_activeBotCount.store(active);
    m_dockedBotCount.store(docked);
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

    // Random/unspecified spawn (charID==0): always REUSE an already-created pilot
    // from the persistent pool (chrCharacters accountID=0 + botMemory) when any
    // exist. New legends are only rolled to TOP UP a fresh/empty pool — never past
    // playerBots.MaxTotalPilots — so the population is a stable set of ~N pilots
    // that respawn, not an ever-growing pile of new characters.
    bool reuseExisting = false;
    uint32 poolEveID = 0;   // ESI portrait source for pooled pilots (botPortraits)
    if (useCharID == 0 && useName.empty()) {
        // 1) ALWAYS try to reuse an established pool pilot first. This used to be
        //    gated on a separate COUNT query (`if (poolCount > 0)`); if that count
        //    failed/returned 0 under DB load, reuse was skipped and the spawner
        //    minted a brand-new character every time — the pool grew past the cap.
        {
            DBQueryResult bres;
            if (sDatabase.RunQuery(bres,
                "SELECT c.characterName, c.corporationID, cc.allianceID, p.eveCharID"
                " FROM chrCharacters c"
                " JOIN botMemory b ON b.charID = c.characterID"
                " LEFT JOIN crpCorporation cc ON cc.corporationID = c.corporationID"
                " LEFT JOIN botPortraits p ON p.serverCharID = c.characterID"
                " WHERE c.characterName != ''"
                " ORDER BY RAND() LIMIT 1"))
            {
                DBResultRow brow;
                if (bres.GetRow(brow)) {
                    useName = brow.GetText(0);
                    useCorpID = brow.GetUInt(1);
                    useAllianceID = brow.GetUInt(2);
                    poolEveID = brow.GetUInt(3);
                    reuseExisting = true;
                    _log(BOT__TRACE, "BotMgr: reusing established bot '%s' (corp %u, ally %u).",
                         useName.c_str(), useCorpID, useAllianceID);
                }
            }
        }

        // 2) Empty pool → top up to the cap only. On a failed count assume the cap
        //    is reached (never risk creating past MaxTotalPilots).
        if (!reuseExisting) {
            uint32 cap = sConfig.playerBots.MaxTotalPilots;
            uint32 poolCount = 0;
            bool countOk = false;
            DBQueryResult cres;
            if (sDatabase.RunQuery(cres,
                "SELECT COUNT(*) FROM chrCharacters WHERE accountID = 0 AND characterName != ''")) {
                DBResultRow crow;
                if (cres.GetRow(crow)) { poolCount = crow.GetUInt(0); countOk = true; }
            }
            if (!countOk || (cap > 0 && poolCount >= cap)) {
                _log(BOT__TRACE, "BotMgr: pilot pool at cap (%u/%u) — not creating another.",
                     poolCount, cap);
                return;
            }
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
        if (reuseExisting) {
            // pick another established pilot from the pool
            DBQueryResult rres;
            if (!sDatabase.RunQuery(rres,
                "SELECT c.characterName, c.corporationID, cc.allianceID, p.eveCharID"
                " FROM chrCharacters c"
                " JOIN botMemory b ON b.charID = c.characterID"
                " LEFT JOIN crpCorporation cc ON cc.corporationID = c.corporationID"
                " LEFT JOIN botPortraits p ON p.serverCharID = c.characterID"
                " WHERE c.characterName != ''"
                " ORDER BY RAND() LIMIT 1"))
                break;
            DBResultRow rrow;
            if (!rres.GetRow(rrow))
                break;
            useName = rrow.GetText(0);
            useCorpID = rrow.GetUInt(1);
            useAllianceID = rrow.GetUInt(2);
            poolEveID = rrow.GetUInt(3);
            useShipType = 0;
            useFit.clear();
        } else {
            // Fresh top-up retry: never overshoot the cap (each retry would
            // otherwise mint another character).
            uint32 cap = sConfig.playerBots.MaxTotalPilots;
            if (cap > 0) {
                DBQueryResult ccres;
                uint32 cnt = 0;
                bool ok = false;
                if (sDatabase.RunQuery(ccres,
                    "SELECT COUNT(*) FROM chrCharacters WHERE accountID = 0 AND characterName != ''")) {
                    DBResultRow ccrow;
                    if (ccres.GetRow(ccrow)) { cnt = ccrow.GetUInt(0); ok = true; }
                }
                if (!ok || cnt >= cap)
                    break;
            }
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
        }
        // NOTE: do NOT set useCharID from the legend here — CreateBotCharacter
        // allocates the real charID on the next loop iteration.
    }
    if (botAlreadyHere) {
        _log(BOT__TRACE, "BotMgr: all candidate legends already in system %u — skipping spawn.",
             pSystem->GetID());
        return;
    }
    // Pooled (reused) pilots keep growing their skillbook across respawns.
    if (reuseExisting)
        CharacterDB::EnsureExtendedBotSkills(useCharID, skillTier < 5 ? (uint8)(skillTier + 1) : 5);
    // Remember the EVE portrait source so fetch_bot_portraits.py can grab it —
    // AND download it now (async) so the client sees a face immediately.
    // Pooled pilots were minted from legends in earlier sessions, so their
    // portrait source comes from the botPortraits map (poolEveID); freshly
    // rolled legends carry their real EVE id in useCharID directly.
    if (poolEveID != 0 && useCharID != poolEveID)
        killmailCharID = poolEveID;
    if (killmailCharID != 0 && killmailCharID != useCharID) {
        DBerror perr;
        sDatabase.RunQuery(perr,
            "INSERT IGNORE INTO botPortraits (serverCharID, eveCharID) VALUES (%u, %u)",
            useCharID, killmailCharID);
        FetchPortraitAsync(useCharID, killmailCharID);
    }

    // Ship hull placeholder — final profession hull is chosen further down and
    // persisted to botMemory afterwards (used by the portal when the bot is offline).
    uint32 hullType = useShipType;

    // Profession: keep the bot's saved job across respawns (a miner stays a miner —
    // it's been learning it). Only brand-new pilots roll a fresh one. Skill tier
    // is loaded the same way: a persisted, levelled-up tier survives respawns.
    PlayerBot::BotProfession prof = PlayerBot::BotProfession::Miner;
    uint8 savedSkill = 0xFF;   // 0xFF = unset → roll fresh below
    {
        DBQueryResult pres;
        if (sDatabase.RunQuery(pres,
            "SELECT profession, skillLevel FROM botMemory WHERE charID = %u AND profession != 255",
            useCharID))
        {
            DBResultRow prow;
            if (pres.GetRow(prow)) {
                prof = (PlayerBot::BotProfession)prow.GetUInt(0);
                savedSkill = (uint8)prow.GetUInt(1);
                if (savedSkill <= 5)
                    skillTier = savedSkill;   // veteran keeps its trained tier
                else {
                    // Legacy pilot with no stored tier — seed it now so it stays
                    // stable across spawns (and can level up from here).
                    DBerror uerr;
                    sDatabase.RunQuery(uerr,
                        "UPDATE botMemory SET skillLevel = %u WHERE charID = %u",
                        skillTier, useCharID);
                }
            } else {
                // New pilot — roll a profession and persist it for future respawns.
                float p = MakeRandomFloat();
                if (p < 0.10f)
                    prof = PlayerBot::BotProfession::Hunter;       // PvP pirates / war corps / guards
                else if (p < 0.24f)
                    prof = PlayerBot::BotProfession::RatHunter;    // peaceful PvE (red crosses only)
                else if (p < 0.46f)
                    prof = PlayerBot::BotProfession::Miner;        // miners (co-op with guards)
                else if (p < 0.56f)
                    prof = PlayerBot::BotProfession::Trader;       // market / station traders
                else if (p < 0.72f)
                    prof = PlayerBot::BotProfession::Courier;      // couriers: haul to/from hub
                else if (p < 0.82f)
                    prof = PlayerBot::BotProfession::Hacker;       // data/relic sites
                else if (p < 0.88f)
                    prof = PlayerBot::BotProfession::Missioner;    // agent mission runners
                else if (p < 0.93f)
                    prof = PlayerBot::BotProfession::Explorer;     // probes / wormholes
                else
                    prof = PlayerBot::BotProfession::Industrialist; // producers/builders (POS, PI, logistics)
                DBerror perr;
                sDatabase.RunQuery(perr,
                    "INSERT INTO botMemory (charID, shipTypeID, profession, skillLevel, lastUpdate)"
                    " VALUES (%u, %u, %u, %u, NOW())"
                    " ON DUPLICATE KEY UPDATE shipTypeID = VALUES(shipTypeID), profession = VALUES(profession),"
                    "  skillLevel = VALUES(skillLevel), lastUpdate = NOW()",
                    useCharID, hullType, (uint8)prof, skillTier);
            }
        }
    }

    // A chelobot's corp and alliance are ALWAYS the real ones recorded on its
    // character row (chrCharacters.corporationID -> crpCorporation.allianceID) —
    // never the killmail legend's ids. The legend (zkillboard/sotzone) carries
    // live-EVE corp/alliance ids that don't exist in this server's
    // crpCorporation/alnAlliance, and the client hard-crashes/hangs rendering
    // them (character sheet killmail + standings windows). NPC starter corps
    // have allianceID 0, exactly like a real player fresh out of a school; a bot
    // only gets an alliance when it actually goes through the player path
    // (MaybeFoundCorp -> MaybeFormAlliance creates a real alnAlliance row and
    // stamps allianceID on the corp). Nothing is invented here.
    {
        DBQueryResult cq;
        if (sDatabase.RunQuery(cq,
            "SELECT ch.corporationID, cc.allianceID"
            " FROM chrCharacters ch"
            " LEFT JOIN crpCorporation cc ON cc.corporationID = ch.corporationID"
            " WHERE ch.characterID = %u", useCharID)) {
            DBResultRow crow;
            if (cq.GetRow(crow)) {
                useCorpID = crow.GetUInt(0);
                useAllianceID = crow.GetUInt(1);
                _log(BOT__TRACE, "BotMgr: %s(%u) corp %u alliance %u (from character row).",
                     useName.c_str(), useCharID, useCorpID, useAllianceID);
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
    bool spawnFleetBoss = false;   // experienced miner flying an Orca/Rorqual
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
                // Professional mining fleet: a practised miner (high skill tier)
                // sometimes brings the boss — an Orca (or Rorqual in null) that
                // boosts nearby barges of its corp. The rest stay on barges.
                if (skillTier >= 3 && MakeRandomInt(0, 99) < 20) {
                    bool isNull = pSystem->GetSystemSecurityRating() < 0.0f;
                    hullType = isNull ? 28352 : 28606;   // Rorqual / Orca
                    forceProfessionHull = false;
                    spawnFleetBoss = true;
                }
                break;
            case PlayerBot::BotProfession::Trader:
            case PlayerBot::BotProfession::Courier:
                pick = haulerHulls; pickCount = sizeof(haulerHulls)/sizeof(haulerHulls[0]); forceProfessionHull = true;
                break;
            case PlayerBot::BotProfession::Hacker:
            case PlayerBot::BotProfession::Explorer:
                pick = scanHulls; pickCount = sizeof(scanHulls)/sizeof(scanHulls[0]); forceProfessionHull = true;
                break;
            case PlayerBot::BotProfession::Missioner:
                // Agent mission runner — a standard combat hull (cruiser/BC), the
                // kind of ship an agent contract pilot actually uses.
                pick = combatHulls; pickCount = sizeof(combatHulls)/sizeof(combatHulls[0]); forceProfessionHull = true;
                break;
            case PlayerBot::BotProfession::Industrialist:
                // Producer/builder flies an industrial hauler (moves its own goods
                // between the POS/station and the market).
                pick = haulerHulls; pickCount = sizeof(haulerHulls)/sizeof(haulerHulls[0]); forceProfessionHull = true;
                break;
            default:   // Hunter / RatHunter — combat
                pick = combatHulls; pickCount = sizeof(combatHulls)/sizeof(combatHulls[0]);
                break;
        }

        if (spawnFleetBoss) {
            // Boss hull already chosen (Orca/Rorqual) — keep it, no re-pick.
        } else if (forceProfessionHull) {
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

    // Keep the bot's persistent "typical" hull in sync every spawn (used by the
    // portal to show a ship even when the bot is offline and its hull NPC is gone).
    // Runs AFTER the profession-hull pick above so it stores the real flying hull.
    {
        DBerror merr;
        sDatabase.RunQuery(merr,
            "UPDATE botMemory SET shipTypeID = %u, lastUpdate = NOW() WHERE charID = %u",
            hullType, useCharID);
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
        // Non-combat hulls (mining barges/exhumers, industrials, freighters,
        // transports, shuttles, capsules...) have no guns — a hauler/barge must
        // NOT deal combat damage. Peaceful pilots on real combat hulls still fight
        // back with their weapons. A real pilot in a freighter warps out, it does
        // not "shoot back" with a cargo bay.
        using namespace EVEDB::invGroups;
        switch (grp) {
            case MiningBarge: case Exhumer: case Industrial:
            case Freighter: case TransportShip: case JumpFreighter:
            case CapitalIndustrialShip: case Shuttle: case Capsule:
            case IndustrialCommandShip:
                base = 0.0f;
                break;
            default:
                break;
        }
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
    // Real killmail fit: materialize the fitted modules (from the legend's
    // fitted_item_ids) as actual items in the ship's slots — the client shows the
    // genuine fit and a wreck drops real module loot, like a player's lossmail.
    // Only when the bot is actually flying the legend hull (combat professions
    // keep the killmail ship). Professional hulls (miner barges, haulers, scan
    // frigates) are force-picked per profession above and would mismatch a combat
    // legend's fit — those bots run a profession fit instead, not a lossmail one.
    if (hullType == useShipType && !useFit.empty()) {
        // After a loss the pilot must re-BUY the fit on the open market with its
        // own ISK (upgraded as far as its skill tier + wallet allow), exactly like
        // a real player who lost a ship. New/undocked spawns that were never
        // killed keep the free legend fit.
        uint32 stationID = 0;
        for (auto& [sid, sse] : pSystem->GetStaticEntities())
            if (sse != nullptr && sse->GetStationSE() != nullptr) { stationID = sse->GetID(); break; }
        bool resupply = false;
        uint32 deaths = 0;
        if (stationID != 0) {
            DBQueryResult mres;
            if (sDatabase.RunQuery(mres,
                "SELECT deaths, resuppliedDeaths FROM botMemory WHERE charID = %u", useCharID))
            {
                DBResultRow mrow;
                if (mres.GetRow(mrow)) {
                    deaths = mrow.GetUInt(0);
                    resupply = deaths > mrow.GetUInt(1);
                }
            }
        }
        if (resupply) {
            std::string boughtFit = ResupplyBotFit(useCharID, stationID, skillTier, useFit);
            if (!boughtFit.empty()) {
                MaterializeBotFit(iRef, useCharID, boughtFit, stationID);
                DBerror uerr;
                sDatabase.RunQuery(uerr,
                    "UPDATE botMemory SET resuppliedDeaths = %u WHERE charID = %u", deaths, useCharID);
            } else {
                // Can't afford even a bare hull — the pilot undocks stripped and
                // has to earn the ISK back before it can fight properly again.
                _log(BOT__MESSAGE, "BotMgr: pilot %u cannot afford to re-fit after loss — undocking stripped.",
                     useCharID);
            }
        } else {
            MaterializeBotFit(iRef, useCharID, useFit);
        }
    }
    // Ammo/charges (T1/T2 by skill tier) + small profession-typical cargo.
    MaterializeShipLoad(iRef, useCharID, (uint8)prof, skillTier);
    // The bot's combat/profession tier comes from its persisted skillLevel
    // (levelled up by practice), not the ctor default of 3.
    bot->SetBotSkillLevel(skillTier);
    bot->SetFleetBoss(spawnFleetBoss);
    bot->GetAIMgr()->SetAmbush(false);   // bots are not ambushing rats
    bot->DestinyMgr()->SetPosition(pos);
    pSystem->AddNPC(bot);

    // Chelobots are real pilots as far as the board is concerned: point their
    // chrCharacters row at the ship they are flying now (same as a player's
    // active ship) so lists/portals show the hull. Reset on despawn.
    {
        DBerror perr;
        sDatabase.RunQuery(perr,
            "UPDATE chrCharacters SET shipID = %u, solarSystemID = %u, stationID = 0, online = 1 WHERE characterID = %u",
            iRef->itemID(), pSystem->GetID(), useCharID);
    }

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

// ---- procedural portrait generator (fallback when ESI is unreachable) ----
// Minimal PNG encoder (8-bit RGB, no interlace) + a deterministically randomised
// "pilot bust": nebula-gradient backdrop, head/shoulders silhouette, one of
// several skin/hair/shirt palettes. The same seed always renders the same
// generated portrait, so a given pilot keeps its face across regenerations.
// zlib (compress2/crc32) is already linked via eve-core's Deflate utils.

#include <zlib.h>

// PNG chunk: length + type + data + crc32(type+data)
static void PngChunk(std::vector<uint8>& png, const char* type,
                     const uint8* data, size_t len)
{
    uint32 n = (uint32)len;
    png.push_back((n >> 24) & 0xFF); png.push_back((n >> 16) & 0xFF);
    png.push_back((n >> 8) & 0xFF);  png.push_back(n & 0xFF);
    size_t hdr = png.size();
    for (int i = 0; i < 4; ++i) png.push_back((uint8)type[i]);
    png.insert(png.end(), data, data + len);
    uint32 crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, &png[hdr], (uInt)(png.size() - hdr));
    png.push_back((crc >> 24) & 0xFF); png.push_back((crc >> 16) & 0xFF);
    png.push_back((crc >> 8) & 0xFF);  png.push_back(crc & 0xFF);
}

bool BotMgr::GeneratePortraitPNG(const std::string& path, uint32 seed)
{
    const int W = 512, H = 512;
    // deterministic LCG: identical seed -> identical image
    uint32 rngState = seed * 2654435761u + 0x9E3779B9u;
    auto nxt = [&]() -> uint32 {
        rngState = rngState * 1664525u + 1013904223u;
        return rngState >> 16;
    };
    auto rndRange = [&](int a, int b) -> int {
        return a + (int)(nxt() % (uint32)(b - a + 1));
    };

    // ---- palette ----
    int bgA[3] = { rndRange(16, 40), rndRange(26, 56), rndRange(44, 84) };
    int bgB[3] = { bgA[0] + rndRange(4, 26), bgA[1] + rndRange(6, 34), bgA[2] + rndRange(10, 46) };
    static const int skins[8][3] = {
        {233,190,157},{205,160,120},{210,180,150},{190,140,110},
        {170,120,95},{240,200,170},{150,105,80},{255,220,190}
    };
    const int* skin = skins[rndRange(0, 7)];
    int skinDk[3] = { skin[0] * 8 / 10, skin[1] * 8 / 10, skin[2] * 8 / 10 };
    static const int hairs[6][3] = {
        {40,30,24},{90,60,40},{150,110,60},{190,160,120},{70,55,45},{120,40,30}
    };
    const int* hair = hairs[rndRange(0, 5)];
    static const int shirts[8][3] = {
        {70,90,140},{140,80,60},{60,110,90},{120,90,150},{110,110,120},
        {180,150,60},{90,70,110},{60,140,120}
    };
    const int* shirt = shirts[rndRange(0, 7)];

    // ---- geometry ----
    float hcx = (float)rndRange(W * 42, W * 58) / 100.0f;
    float hcy = (float)rndRange(H * 30, H * 40) / 100.0f * H;
    float hrx = (float)rndRange(W * 10, W * 15) / 100.0f;
    float hry = hrx * 1.28f;
    float shY  = hcy + hry * 1.06f;
    float bodyW = (float)rndRange(36, 50) / 100.0f;
    float gx = W / 2.0f, gy = hcy - hry * 0.4f;
    float gr = (float)rndRange(W, W + W / 2);

    std::vector<uint8> raw((size_t)(1 + W * 3) * H);
    for (int y = 0; y < H; ++y) {
        uint8* row = &raw[(size_t)y * (1 + W * 3)];
        row[0] = 0;   // PNG filter: none
        uint8* px = &row[1];
        float fy = (float)y / (float)H;
        for (int x = 0; x < W; ++x) {
            float c[3];
            for (int i = 0; i < 3; ++i)
                c[i] = (float)(bgA[i] + (bgB[i] - bgA[i]) * fy);
            // nebula glow behind the head
            {
                float dx = x - gx, dy = y - gy;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < gr) {
                    float k = 1.0f - dist / gr;
                    k *= k;
                    for (int i = 0; i < 3; ++i)
                        c[i] += k * 26.0f;
                }
            }
            // shoulders/torso: rounded trapezoid widening downward
            bool inTorso = false;
            if (y > shY) {
                float widen = (y - shY) / (float)(H - shY);
                float halfW = bodyW * W * (0.42f + 0.58f * widen);
                if (fabsf((float)x - hcx * W) < halfW)
                    inTorso = true;
            }
            // circular neck/edge underline of the shirt collar
            bool inHead = false;
            float nx = (x - hcx * W) / hrx;
            float ny = (y - hcy) / hry;
            float e = nx * nx + ny * ny;
            if (e <= 1.0f)
                inHead = true;

            int col[3];
            const int* faceCol = (y < hcy + hry * 0.12f && e <= 1.0f) ? skin : skinDk;
            if (inHead) {
                // hair cap: top third of the head ellipse
                if (e <= 1.0f && y < hcy - hry * 0.25f)
                    for (int i = 0; i < 3; ++i) col[i] = hair[i];
                else
                    for (int i = 0; i < 3; ++i) col[i] = faceCol[i];
            } else if (inTorso) {
                for (int i = 0; i < 3; ++i) col[i] = shirt[i];
            } else {
                for (int i = 0; i < 3; ++i) col[i] = (int)c[i];
            }
            for (int i = 0; i < 3; ++i) {
                if (col[i] < 0) col[i] = 0;
                if (col[i] > 255) col[i] = 255;
                px[i] = (uint8)col[i];
            }
            px += 3;
        }
    }

    // ---- zlib deflate ----
    uLongf dstLen = compressBound(raw.size());
    std::vector<uint8> idat(dstLen);
    if (compress2(idat.data(), &dstLen, raw.data(), raw.size(), 6) != Z_OK)
        return false;
    idat.resize(dstLen);

    // ---- assemble PNG ----
    std::vector<uint8> png;
    static const uint8 sig[8] = {137,80,78,71,13,10,26,10};
    png.insert(png.end(), sig, sig + 8);
    {
        uint8 ihdr[13];
        ihdr[0] = (W >> 24) & 0xFF; ihdr[1] = (W >> 16) & 0xFF;
        ihdr[2] = (W >> 8) & 0xFF;  ihdr[3] = W & 0xFF;
        ihdr[4] = (H >> 24) & 0xFF; ihdr[5] = (H >> 16) & 0xFF;
        ihdr[6] = (H >> 8) & 0xFF;  ihdr[7] = H & 0xFF;
        ihdr[8] = 8;   // bit depth
        ihdr[9] = 2;   // color type: truecolor RGB
        ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
        PngChunk(png, "IHDR", ihdr, 13);
    }
    PngChunk(png, "IDAT", idat.data(), idat.size());
    uint8 empty = 0;
    PngChunk(png, "IEND", &empty, 0);

    FILE* f = fopen(path.c_str(), "wb");
    if (f == nullptr)
        return false;
    fwrite(png.data(), 1, png.size(), f);
    fclose(f);
    return true;
}

// Download the bot's ESI portrait into the image cache right now, so the
// client shows a face immediately (no cron lag). Runs the whole chain in a
// forked child so the game loop never blocks. Path:
// <imageDir>/Character/<serverCharID>_512.jpg (ImageServer::GetFilePath).
// Chain: 1) ESI (image.evetech.net is blocked from RU); 2) the same URL via
// the configured proxy; 3) procedural generation with this binary's
// standalone "genportrait" mode — a bot is NEVER left without a face, its
// random portrait is generated onto the image server.
void BotMgr::FetchPortraitAsync(uint32 serverCharID, uint32 eveCharID)
{
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

    const std::string tmp = path + ".tmp";
    std::string proxy = sConfig.telegram.Proxy;

    // Use --fail so a failed request writes nothing, and fetch into .tmp then
    // move — the generated fallback is never destroyed by a failed download.
    std::string sh =
        "curl -sSL --fail --max-time 20 -o '" + tmp + "' '" + url + "'"
        " && [ -s '" + tmp + "' ] && mv '" + tmp + "' '" + path + "' && exit 0";
    if (!proxy.empty())
        sh += " ; curl -sSL --fail --max-time 20 --proxy '" + proxy + "' -o '"
            + tmp + "' '" + url + "'"
            + " && [ -s '" + tmp + "' ] && mv '" + tmp + "' '" + path + "' && exit 0";
    // final fallback: random procedural portrait by this binary itself
    char self[2048];
    ssize_t slen = ::readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (slen > 0) {
        self[slen] = 0;
        sh += " ; '" + std::string(self) + "' genportrait '" + path + "' "
            + std::to_string(serverCharID);
    }

    pid_t pid = ::fork();
    if (pid == 0) {
        // child: /bin/sh -c "<chain>"
        ::execl("/bin/sh", "sh", "-c", sh.c_str(), (char*)nullptr);
        _exit(127);
    }
    // parent: don't wait — let it finish in the background
    _log(BOT__TRACE, "BotMgr: fetching portrait for bot %u (eve %u) -> %s", serverCharID, eveCharID, path.c_str());
}

// Materialize a killmail fit (JSON array of module typeIDs) as REAL item children
// of the bot's ship. Each module is spawned as an actual item in the ship with
// the correct high/mid/low/rig slot flag — the same representation a player's
// fitted hull has in the entity table. Chelobot hulls are NPC ships that never
// get a pilot (ShipItem::SetPlayer -> ModuleManager::Initialize never runs), so
// we do NOT go through ShipItem::AddItemByFlag/ModuleManager (their .at()/pilot
// derefs would crash on an uninitialized manager). Instead slot flags are chosen
// directly from the module's power effect + the ship's slot count, mirroring what
// ModuleManager::Initialize would do when the hull is eventually loaded.
//
// Consumers that already read real ship children benefit automatically:
//   - RecordBotKillMail (PlayerBot.cpp) lists real items in the lossmail, so the
//     kill page shows the genuine fit instead of a synthesized one.
//   - A wreck of this ship can drop the real module loot like a player's wreck.
void BotMgr::MaterializeBotFit(InventoryItemRef shipRef, uint32 charID, const std::string& fitJson, uint32 buyStationID)
{
    if (shipRef.get() == nullptr || charID == 0 || fitJson.empty())
        return;
    ShipItemRef ship = ShipItemRef::StaticCast(shipRef);
    if (ship.get() == nullptr)
        return;

    // fitted_item_ids is "[typeID, typeID, ...]" — parse the integer list (no
    // JSON lib in-tree; the format is a plain array of module typeIDs).
    std::vector<uint32> typeIDs;
    {
        std::string cur;
        for (char c : fitJson) {
            if (isdigit((unsigned char)c))
                cur.push_back(c);
            else if (!cur.empty()) {
                typeIDs.push_back((uint32)strtoul(cur.c_str(), nullptr, 10));
                cur.clear();
            }
        }
        if (!cur.empty())
            typeIDs.push_back((uint32)strtoul(cur.c_str(), nullptr, 10));
    }
    if (typeIDs.empty())
        return;

    // How many slots does this hull actually have? Only fit hulls that can carry
    // anything (skip shuttles/pods/capsules — no fight fit). Slots mirror the
    // ship's dogma attributes (AttrLowSlots/AttrMedSlots/AttrHiSlots/AttrRigSlots).
    uint32 loMax = ship->GetAttribute(AttrLowSlots).get_uint32();
    uint32 midMax = ship->GetAttribute(AttrMedSlots).get_uint32();
    uint32 hiMax = ship->GetAttribute(AttrHiSlots).get_uint32();
    uint32 rigMax = ship->GetAttribute(AttrRigSlots).get_uint32();
    if (loMax == 0 && midMax == 0 && hiMax == 0 && rigMax == 0)
        return;

    // Slot banks (EVE_Flags.h): low 11.., mid 19.., hi 27.., rig 92..
    struct SlotBank { uint16 baseFlag; uint32 used; uint32 max; };
    SlotBank low  = { flagLowSlot0, 0, loMax };
    SlotBank mid  = { flagMidSlot0, 0, midMax };
    SlotBank hi   = { flagHiSlot0, 0, hiMax };
    SlotBank rig  = { flagRigSlot0, 0, rigMax };

    // Account for modules already fitted on this hull (respawn on an existing
    // ship, or a DB-loaded hull). Everything currently under the ship in module
    // slots is counted so new fits don't overwrite occupied slots.
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT flag, COUNT(*) FROM entity WHERE locationID = %u AND flag BETWEEN %u AND %u GROUP BY flag",
            ship->itemID(), (uint32)flagLowSlot0, (uint32)flagRigSlot7))
        {
            DBResultRow row;
            while (res.GetRow(row)) {
                uint32 flag = row.GetUInt(0);
                uint32 cnt  = row.GetUInt(1);
                if (flag >= flagRigSlot0)                  rig.used  += cnt;
                else if (flag >= flagHiSlot0 && flag < flagFixedSlot) hi.used  += cnt;
                else if (flag >= flagMidSlot0 && flag < flagHiSlot0)  mid.used += cnt;
                else if (flag >= flagLowSlot0 && flag < flagMidSlot0) low.used += cnt;
            }
        }
    }

    // Slot picker for one module's bank: returns the next free flag or flagIllegal
    // when that bank is full.
    auto pickSlot = [](SlotBank& bank) -> EVEItemFlags {
        if (bank.max == 0 || bank.used >= bank.max)
            return flagIllegal;
        return (EVEItemFlags)(bank.baseFlag + bank.used++);
    };

    uint32 fitted = 0;
    for (uint32 typeID : typeIDs) {
        if (typeID == 0)
            continue;
        const ItemType* t = sItemFactory.GetType((uint16)typeID);
        if (t == nullptr)
            continue;
        // Only modules belong in slots (charges/ammo in a fit list are dropped —
        // they belong in cargo or loaded later). Rigs are classed as modules too.
        if (t->categoryID() != EVEDB::invCategories::Module)
            continue;

        // Choose the slot bank from the module's power effect (as the client and
        // ShipItem::FindAvailableModuleSlot do).  Turrets/launchers are hi-slot.
        SlotBank* bank = nullptr;
        if (t->HasEffect(EVEEffectID::loPower))       bank = &low;
        else if (t->HasEffect(EVEEffectID::medPower)) bank = &mid;
        else if (t->HasEffect(EVEEffectID::rigSlot))  bank = &rig;
        else if (t->HasEffect(EVEEffectID::hiPower))  bank = &hi;
        else {
            _log(BOT__TRACE, "BotMgr: MaterializeBotFit — type %u has no power-slot effect, skipping.", typeID);
            continue;
        }
        EVEItemFlags slot = pickSlot(*bank);
        if (slot == flagIllegal)
            continue;   // bank full — skip quietly, the rest of the fit still lands

        // Spawn in limbo owned by the pilot, then MOVE into the ship at the slot
        // flag. Move() registers the module in the ship's in-memory inventory
        // (so ShipItem::Delete -> DeleteContents() cleans it up later) AND writes
        // the new location/flag to the entity table, where the lossmail reader
        // (RecordBotKillMail) picks it up — same representation as a player's
        // fitted module. A direct SpawnItem into the ship would bypass the
        // inventory and orphan the module row on ship delete.
        InventoryItemRef iRef;
        if (buyStationID != 0) {
            // After a loss this module was bought on the open market (BotBuyStock
            // minted it into the bot's hangar) — fit that real item instead of
            // conjuring one. Nothing in the hangar (broke / no order) → skip slot.
            DBQueryResult hres;
            if (sDatabase.RunQuery(hres,
                "SELECT itemID FROM entity WHERE ownerID = %u AND locationID = %u"
                "  AND typeID = %u AND flag = %u AND singleton = 0"
                "  ORDER BY itemID LIMIT 1",
                charID, buyStationID, typeID, (uint32)flagHangar))
            {
                DBResultRow hrow;
                if (hres.GetRow(hrow))
                    iRef = sItemFactory.GetItemRef(hrow.GetUInt(0));
            }
        } else {
            ItemData idata((uint16)typeID, charID, locTemp, flagNone, 1);
            iRef = sItemFactory.SpawnItem(idata);
        }
        if (iRef.get() == nullptr)
            continue;
        // Fitted modules are unique items (singleton), as the client expects.
        iRef->ChangeSingleton(true, false);
        iRef->Move(ship->itemID(), slot, false);
        ++fitted;
        _log(BOT__TRACE, "BotMgr: MaterializeBotFit — fitted %s(%u) to flag %u.",
             t->name().c_str(), typeID, (uint32)slot);
    }

    // Top-up: legend fits are real-EVE killboard data, so many module typeIDs are
    // too new for this Crucible server and were skipped above. When the fit ended
    // up with NO usable weapon, give the hull a guaranteed Crucible-valid one (the
    // same weapon the AI fires, read from AttrGfxTurretID) so it isn't stripped.
    if (hi.used == 0 && hiMax > 0 && ship->HasAttribute(AttrGfxTurretID)) {
        uint32 weapon = ship->GetAttribute(AttrGfxTurretID).get_uint32();
        if (weapon > 0 && weapon != (uint32)ship->typeID()) {
            const ItemType* wt = sItemFactory.GetType((uint16)weapon);
            EVEItemFlags slot = pickSlot(hi);
            if (wt != nullptr && slot != flagIllegal) {
                ItemData idata((uint16)weapon, charID, locTemp, flagNone, 1);
                InventoryItemRef wRef = sItemFactory.SpawnItem(idata);
                if (wRef.get() != nullptr) {
                    wRef->ChangeSingleton(true, false);
                    wRef->Move(ship->itemID(), slot, false);
                    ++fitted;
                    _log(BOT__TRACE, "BotMgr: MaterializeBotFit — Crucible fallback weapon %s(%u) to flag %u.",
                         wt->name().c_str(), weapon, (uint32)slot);
                }
            }
        }
    }

    if (fitted > 0)
        _log(BOT__MESSAGE, "BotMgr: materialized %u fitted modules for pilot %u's %s.",
             fitted, charID, ship->name());
}

// The T1→named-meta→T2 ladder for a module, best-first, gated by the pilot's
// simulated skill tier (0..5). The ladder is read from the real SDE tables:
// invMetaTypes links every module variant to its T1 parentTypeID, so a family
// is "everything that hangs off the same root". dgmTypeAttributes carry the
// tech level (422: 1=T1/meta, 2=T2) and meta level (633, 0..5). We let a pilot
// fly:
//   tier 0-1   → T1 only (base or cheap meta 1-3)
//   tier 2-3   → + named meta (1-5)
//   tier 4-5   → + T2
// A vet with money buys T2 first, then the best named meta, then plain T1; a
// broke rookie only ever reaches the bottom of the list.
std::vector<uint32> BotMgr::FitUpgradePath(uint32 baseType, uint8 skillTier)
{
    std::vector<uint32> result;
    if (baseType == 0)
        return result;

    // Climb to the ladder root (the T1 parent), bounded so a broken link can't
    // loop forever.
    uint32 cur = baseType;
    for (int hop = 0; hop < 6; ++hop) {
        uint32 parent = 0;
        DBQueryResult res;
        if (sDatabase.RunQuery(res, "SELECT parentTypeID FROM invMetaTypes WHERE typeID = %u", cur)) {
            DBResultRow row;
            if (res.GetRow(row))
                parent = row.GetUInt(0);
        }
        if (parent == 0 || parent == cur)
            break;
        cur = parent;
    }
    uint32 root = cur;

    struct Cand { uint32 typeID; double meta; double tech; };
    std::vector<Cand> cands;
    cands.push_back({ root, 0.0, 1.0 });   // the T1 base itself
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT v.typeID,"
            "       COALESCE(aM.valueFloat, aM.valueInt, 0),"
            "       COALESCE(aT.valueFloat, aT.valueInt, 0)"
            " FROM invMetaTypes v"
            " JOIN invTypes t ON t.typeID = v.typeID AND t.published = 1 AND t.categoryID = 7"
            " LEFT JOIN dgmTypeAttributes aM ON aM.typeID = v.typeID AND aM.attributeID = 633"
            " LEFT JOIN dgmTypeAttributes aT ON aT.typeID = v.typeID AND aT.attributeID = 422"
            " WHERE v.parentTypeID = %u", root))
        {
            DBResultRow row;
            while (res.GetRow(row))
                cands.push_back({ row.GetUInt(0), row.GetDouble(1), row.GetDouble(2) });
        }
    }

    // Best first: T2 before named meta before T1.
    std::sort(cands.begin(), cands.end(),
        [](const Cand& a, const Cand& b) {
            if (a.tech != b.tech) return a.tech > b.tech;
            if (a.meta != b.meta) return a.meta > b.meta;
            return a.typeID < b.typeID;
        });
    for (const Cand& c : cands) {
        bool usable = true;
        if (c.tech >= 2.0)        usable = (skillTier >= 4);   // T2: vets only
        else if (c.meta >= 4.0)   usable = (skillTier >= 3);   // top named meta
        if (!usable)
            continue;
        bool dup = false;
        for (uint32 t : result)
            if (t == c.typeID) { dup = true; break; }
        if (!dup)
            result.push_back(c.typeID);
    }
    if (result.empty())
        result.push_back(baseType);   // never worse than the original fit entry
    return result;
}

// Re-buy a killed bot's fit on the open market. Every module of the legend fit
// is upgraded along its T1→meta→T2 ladder as far as the pilot's skill tier and
// wallet allow (BotBuyStock debits real ISK and mints the module into the bot's
// hangar at `stationID`). Returns a re-serialised "[typeID, ...]" list of what
// was actually bought — the caller fits those (MaterializeBotFit w/ buyStationID
// pulls the bought items out of the hangar). An empty result means the pilot is
// too broke to replace anything and flies out stripped (no free modules).
std::string BotMgr::ResupplyBotFit(uint32 charID, uint32 stationID, uint8 skillTier, const std::string& fitJson)
{
    if (charID == 0 || stationID == 0 || fitJson.empty())
        return std::string();

    std::vector<uint32> typeIDs;
    {
        std::string cur;
        for (char c : fitJson) {
            if (isdigit((unsigned char)c))
                cur.push_back(c);
            else if (!cur.empty()) {
                typeIDs.push_back((uint32)strtoul(cur.c_str(), nullptr, 10));
                cur.clear();
            }
        }
        if (!cur.empty())
            typeIDs.push_back((uint32)strtoul(cur.c_str(), nullptr, 10));
    }

    std::vector<uint32> bought;
    double spent = 0.0;
    for (uint32 typeID : typeIDs) {
        if (typeID == 0)
            continue;
        const ItemType* t = sItemFactory.GetType((uint16)typeID);
        if (t == nullptr || t->categoryID() != EVEDB::invCategories::Module)
            continue;   // only modules go in slots — ammo/cargo aren't bought here
        std::vector<uint32> path = FitUpgradePath(typeID, skillTier);
        for (uint32 cand : path) {
            double cost = sMktMgr.BotBuyStock(charID, stationID, cand, 1);
            if (cost > 0.0) {
                bought.push_back(cand);
                spent += cost;
                break;
            }
        }
    }
    if (bought.empty())
        return std::string();

    std::string json = "[";
    for (size_t i = 0; i < bought.size(); ++i) {
        if (i) json += ",";
        json += std::to_string(bought[i]);
    }
    json += "]";
    _log(BOT__MESSAGE, "BotMgr: pilot %u re-bought %u modules at station %u for %.0f ISK after loss.",
         charID, (uint32)bought.size(), stationID, spent);
    return json;
}

// After a bot's fit is materialized: give its weapons real ammo/charges (T1 for
// rookies, T2 once the pilot's skill tier is high enough) and put a believable,
// profession-typical cargo in the hold. Real EVE pilots fly with ammo and cargo
// that matches their job — a killed chelobot should drop those too.
void BotMgr::MaterializeShipLoad(InventoryItemRef shipRef, uint32 charID, uint8 profession, uint8 skillTier)
{
    if (shipRef.get() == nullptr || charID == 0)
        return;
    uint32 shipID = shipRef->itemID();

    auto stackToCargo = [&](uint32 typeID, uint32 qty) {
        if (typeID == 0 || qty == 0)
            return;
        ItemData idata((uint16)typeID, charID, locTemp, flagNone, qty);
        InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
        if (iRef.get() == nullptr)
            return;
        iRef->Move(shipID, flagCargoHold, false);
        _log(BOT__TRACE, "BotMgr: MaterializeShipLoad — cargo %u x type %u.", qty, typeID);
    };

    // 1) Ammo for a missile boat (the exact missile the AI fires is in
    //    AttrEntityMissileTypeID). Veterans (skill tier >= 4) carry the T2
    //    version of the same charge.
    if (shipRef->HasAttribute(AttrEntityMissileTypeID)) {
        uint32 baseMissile = shipRef->GetAttribute(AttrEntityMissileTypeID).get_uint32();
        if (baseMissile > 0) {
            uint32 chargeID = baseMissile;
            if (skillTier >= 4) {
                // Scourge Light Missile -> Scourge Light Missile II (same DB row)
                std::string t2Name;
                DBQueryResult nres;
                if (sDatabase.RunQuery(nres, "SELECT typeName FROM invTypes WHERE typeID = %u", baseMissile)) {
                    DBResultRow nrow;
                    if (nres.GetRow(nrow)) {
                        std::string base = nrow.GetText(0);
                        // base is '... Light Missile' -> T2 '... Light Missile II'
                        t2Name = base + " II";
                    }
                }
                if (!t2Name.empty()) {
                    DBQueryResult r2;
                    if (sDatabase.RunQuery(r2, "SELECT typeID FROM invTypes WHERE typeName = '%s' LIMIT 1", t2Name.c_str())) {
                        DBResultRow rrow;
                        if (r2.GetRow(rrow))
                            chargeID = rrow.GetUInt(0);
                    }
                }
            }
            stackToCargo(chargeID, MakeRandomInt(300, 800));
        }
    }

    // 1b) Charges for TURRETS (lasers use frequency crystals, hybrids/projectiles
    //     use ammo). Read the fitted high-slot weapons' chargeGroup1 (attr 604)
    //     and carry a real charge of that group in the hold.
    {
        DBQueryResult cgRes;
        if (sDatabase.RunQuery(cgRes,
            "SELECT DISTINCT a.valueInt FROM entity e "
            " JOIN dgmTypeAttributes a ON a.typeID = e.typeID AND a.attributeID = 604 "
            " WHERE e.locationID = %u AND e.flag BETWEEN 27 AND 34 AND a.valueInt > 0", shipID)) {
            DBResultRow cgRow;
            std::set<uint32> groups;
            while (cgRes.GetRow(cgRow))
                groups.insert(cgRow.GetUInt(0));
            for (uint32 g : groups) {
                uint32 chargeID = 0;
                DBQueryResult cRes;
                // prefer a T2 charge for veterans, else a plain T1 one
                if (skillTier >= 4 &&
                    sDatabase.RunQuery(cRes,
                        "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 "
                        " AND typeName LIKE '%% II' ORDER BY RAND() LIMIT 1", g)) {
                    DBResultRow cRow;
                    if (cRes.GetRow(cRow)) chargeID = cRow.GetUInt(0);
                }
                if (chargeID == 0 &&
                    sDatabase.RunQuery(cRes,
                        "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 "
                        " ORDER BY RAND() LIMIT 1", g)) {
                    DBResultRow cRow;
                    if (cRes.GetRow(cRow)) chargeID = cRow.GetUInt(0);
                }
                if (chargeID != 0)
                    stackToCargo(chargeID, MakeRandomInt(100, 400));
            }
        }
    }

    // 1c) If the pilot has Thermodynamics (overheat) trained, they may carry
    //     Nanite Repair Paste ("термопаста") to repair burnt modules.
    {
        DBQueryResult skRes;
        if (sDatabase.RunQuery(skRes,
            "SELECT COUNT(*) FROM entity WHERE ownerID = %u AND flag = 7 "
            " AND typeID = (SELECT typeID FROM invTypes WHERE typeName = 'Thermodynamics' LIMIT 1)", charID)) {
            DBResultRow skRow;
            if (skRes.GetRow(skRow) && skRow.GetUInt(0) > 0)
                stackToCargo(28668, MakeRandomInt(20, 120)); // Nanite Repair Paste
        }
    }

    // 2) Profession-typical cargo. Miners/ratters already carry real ore/loot in
    //    m_cargo during a run — this seeds a baseline so the hold isn't empty the
    //    moment they leave the station.
    using P = PlayerBot::BotProfession;
    switch ((P)profession) {
        case P::Hunter:
        case P::RatHunter:   break;   // combat loadout only — no junk in the hold
        case P::Miner: {
            // A handful of common minerals (Tritanium/Pyerite/Mexallon/Isogen).
            static const uint32 mins[] = { 34, 35, 36, 37 };
            stackToCargo(mins[MakeRandomInt(0, 3)], MakeRandomInt(50, 400));
        } break;
        case P::Hacker:
        case P::Explorer: {
            // Relic fragments / data cores for the site runners.
            static const uint32 relics[] = { 30187, 30558, 30562, 30599, 30600, 30605 };
            stackToCargo(relics[MakeRandomInt(0, 5)], MakeRandomInt(1, 8));
        } break;
        case P::Courier:
        case P::Trader: {
            // Sample trade goods to carry (ore/minerals a hauler moves to market).
            static const uint32 goods[] = { 34, 35, 36, 37, 1230, 1231, 1232 };
            stackToCargo(goods[MakeRandomInt(0, 6)], MakeRandomInt(40, 300));
        } break;
        default: break;
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
        DBerror perr;
        sDatabase.RunQuery(perr,
            "UPDATE chrCharacters SET shipID = 0, solarSystemID = 0, stationID = 0, online = 0 WHERE characterID = %u",
            bot->GetBotCharID());
        bot->Delete();
    }
    // Also drop the docked list for this (now empty) system.
    m_docked.erase(pSystem->GetID());
}

static PlayerBot* BotMgr_FindInSystem(SystemManager* sm, uint32 charID);
static void BotMgr_RequestCourierDock(PlayerBot* pb);

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
            uint32 charID = pb->GetBotCharID();
            std::string name = pb->GetBotName();
            uint32 corp = pb->GetBotCorpID();
            uint32 ally = pb->GetBotAllianceID();
            uint32 curSys = pSystem->GetID();

            // Physical courier haul: keep following the remaining route toward the
            // contract's destination system (gate by gate) instead of wandering.
            auto haulIt = m_hauls.find(charID);
            uint32 destSystem = pb->GetTravelDestination();
            if (haulIt != m_hauls.end() && haulIt->second.arrivedAt == 0) {
                CourierHaul& haul = haulIt->second;
                if (curSys == haul.endSys) {
                    // Already on the destination system but somehow wants to leave —
                    // let it dock & deliver instead of roaming on.
                    haul.arrivedAt = time(nullptr);
                    destSystem = 0;
                    pb->ClearTravel();
                    BotMgr_RequestCourierDock(pb);
                    continue;
                }
                if (!haul.route.empty()) {
                    destSystem = haul.route.front();
                    haul.route.erase(haul.route.begin());
                }
            }

            if (destSystem == 0) {
                if (pb->IsAggressed())
                    continue;
                // Trade-inclined bots (traders, couriers, miners hauling ore,
                // hackers/ratters hauling loot) route toward the primary market
                // hub (Jita) to sell/buy.
                uint32 hub = GetTradeHubSystem();
                bool wantsHub = (pb->GetProfession() == PlayerBot::BotProfession::Trader
                              || pb->GetProfession() == PlayerBot::BotProfession::Courier
                              || pb->GetProfession() == PlayerBot::BotProfession::Miner
                              || pb->GetProfession() == PlayerBot::BotProfession::Hacker
                              || pb->GetProfession() == PlayerBot::BotProfession::RatHunter
                              || pb->GetProfession() == PlayerBot::BotProfession::Explorer);
                if (wantsHub && hub != 0 && hub != pSystem->GetID() && MakeRandomInt(0, 99) < 70)
                    destSystem = hub;
                else
                    destSystem = GetRandomAdjacentSystem(pSystem->GetID());
            }
            if (destSystem == 0)
                continue;   // dead-end system or no map data — stay put

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

            // Courier haul bookkeeping for this hop.
            if (haulIt != m_hauls.end()) {
                CourierHaul& haul = haulIt->second;
                if (haul.route.empty() && destSystem == haul.endSys) {
                    // Reached the destination system. Approach the station and
                    // dock; delivery fires when the bot is actually docked there.
                    haul.arrivedAt = time(nullptr);
                    if (dest != nullptr) {
                        PlayerBot* arrived = BotMgr_FindInSystem(dest, charID);
                        if (arrived != nullptr) {
                            arrived->ClearTravel();
                            arrived->RequestDock();
                        }
                    }
                } else if (haul.route.empty() && !haul.arrivedAt) {
                    // Ran out of route without reaching the destination — drop the run.
                    haulIt = m_hauls.erase(haulIt);
                } else if (!haul.route.empty()) {
                    // Intermediate hop — keep the run moving: warp the courier to
                    // the next gate so ProcessTravel crosses it on the following tick.
                    PlayerBot* arrived = (dest != nullptr) ? BotMgr_FindInSystem(dest, charID) : nullptr;
                    if (arrived != nullptr && !arrived->WantsToTravel() && !arrived->IsTraveling()) {
                        uint32 nextSys = haul.route.front();
                        arrived->SetTravelDestination(nextSys);
                        arrived->MarkForTravel(nextSys);
                    }
                }
            }

            // Non-haul crossings (ordinary wanderers, jump-freighter jumps) deliver
            // instantly on arrival; gate-to-gate hauls deliver on dock or fallback.
            if (haulIt == m_hauls.end())
                CompleteContract(charID, destSystem);
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

// Cached system adjacency (mapSolarSystemJumps). Loaded lazily per system.
std::vector<uint32> BotMgr::GetAdjacentSystems(uint32 systemID)
{
    static std::map<uint32, std::vector<uint32>> cache;
    static std::set<uint32> loaded;
    if (systemID == 0)
        return {};
    if (!loaded.count(systemID)) {
        std::vector<uint32> v;
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT toSolarSystemID FROM mapSolarSystemJumps WHERE fromSolarSystemID = %u", systemID)) {
            DBResultRow row;
            while (res.GetRow(row))
                v.push_back(row.GetUInt(0));
        }
        cache[systemID] = std::move(v);
        loaded.insert(systemID);
    }
    return cache[systemID];
}

// BFS shortest path from..to over the jump graph. Returns false when no path.
bool BotMgr::ComputeHaulRoute(uint32 fromSys, uint32 toSys, std::vector<uint32>& out)
{
    out.clear();
    if (fromSys == 0 || toSys == 0)
        return false;
    if (fromSys == toSys) {
        out.push_back(toSys);
        return true;
    }
    std::set<uint32> seen;
    std::queue<uint32> q;
    std::map<uint32, uint32> prev;
    q.push(fromSys);
    seen.insert(fromSys);
    while (!q.empty()) {
        uint32 cur = q.front(); q.pop();
        for (uint32 n : GetAdjacentSystems(cur)) {
            if (seen.count(n))
                continue;
            seen.insert(n);
            prev[n] = cur;
            if (n == toSys) {
                // reconstruct fromSys -> ... -> toSys
                std::vector<uint32> rev;
                for (uint32 x = toSys; x != fromSys && prev.count(x); x = prev[x])
                    rev.push_back(x);
                rev.push_back(fromSys);
                std::reverse(rev.begin(), rev.end());
                out = rev;
                return true;
            }
            q.push(n);
        }
    }
    return false;
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
                // Traders read the book and actually MOVE the market: arbitrage
                // any crossing spread they see, otherwise quote tighter than the
                // current best bid/ask. Old random-order path removed.
                ProcessDockedTraderEconomy(sysID, db.stationID, db);
                // At the hub, sell any real stock (hauled in) for ISK; elsewhere,
                // pack stock into a courier job to the hub (fall back to virtual).
                if (IsTradeHub(sysID))
                    SellStockAtHub(sysID, db.stationID, db.charID);
                else if (PlaceStockCourierContractAt(sysID, db.stationID, db.charID, db.corpID) == 0)
                    PlaceBotCourierContractAt(sysID, db.charID, db.corpID);
            } else if (prof == (uint8)PlayerBot::BotProfession::Industrialist) {
                // Producer/builder: run the manufacturing chain, then move the
                // output to the hub (courier) or sell it if already at the hub.
                ProcessDockedIndustrialEconomy(sysID, db.stationID, db);
                if (MakeRandomInt(0, 99) < 20)
                    PlaceBotBuyOrderAt(sysID, db.charID, prof);
                if (IsTradeHub(sysID))
                    SellStockAtHub(sysID, db.stationID, db.charID);
                else
                    PlaceStockCourierContractAt(sysID, db.stationID, db.charID, db.corpID);
            } else if (prof == (uint8)PlayerBot::BotProfession::Miner
                       || prof == (uint8)PlayerBot::BotProfession::RatHunter
                       || prof == (uint8)PlayerBot::BotProfession::Hacker
                       || prof == (uint8)PlayerBot::BotProfession::Explorer) {
                // Producers bid for the raw materials they consume (from the dock),
                // and ship the real ore/loot they've banked in the hangar to the
                // hub — or, docked at the hub, sell it for ISK directly.
                if (MakeRandomInt(0, 99) < 20)
                    PlaceBotBuyOrderAt(sysID, db.charID, prof);
                if (IsTradeHub(sysID))
                    SellStockAtHub(sysID, db.stationID, db.charID);
                else
                    PlaceStockCourierContractAt(sysID, db.stationID, db.charID, db.corpID);
            }
        }
    }
}

// Market self-learning for a docked trader. Every few minutes the bot reads the
// order book at its own station and tries to make money on the spread:
//   - if any OTHER order has crossed the book (best bid above best ask) it
//     executes a real arbitrage fill (MarketMgr::BotArbitrageFill) — buying at
//     the ask and selling at the bid. Volume is consumed, mktTransactions are
//     written and the profit lands in the bot's wallet.
//   - if the book is clean it quotes tighter than the current best bid/ask
//     (market-making): its resting orders invite the next trade to land on it.
// Self-learning: each fill result is fed back into botMemory.tradeProfit; a bot
// that keeps making money quotes tight and trades bolder, one that keeps losing
// widens its required margin and stops chasing the market.
void BotMgr::ProcessDockedTraderEconomy(uint32 sysID, uint32 stationID, const DockedBot& db)
{
    if (sysID == 0 || db.charID == 0)
        return;
    if (stationID == 0) {
        // Fallback (rare): the first station in the system, matching the old path.
        DBQueryResult res;
        if (sDatabase.RunQuery(res, "SELECT stationID FROM staStations WHERE solarSystemID = %u LIMIT 1", sysID)) {
            DBResultRow row;
            if (res.GetRow(row))
                stationID = row.GetUInt(0);
        }
    }
    if (stationID == 0)
        return;

    BotMemory mem(db.charID);
    mem.Load();
    float confidence = mem.GetTradeConfidence();   // -1..1 from tradeProfit/tradeLosses

    // A trader works a couple of goods it knows (minerals/ammo/common modules).
    // Includes the datacores / decryptors / salvage that hacker & explorer bots
    // pull from data/relic sites — so the hub has real buy orders for that loot
    // and the physical-goods ISK loop closes for hackers too (SellStockAtHub).
    static const uint32 tradeGoods[] = {
        34, 38, 39, 40, 1229, 1230, 1231, 1232, 2048, 3775, 2676, 2488,
        // datacores (group 333) — hacker loot
        20171, 20172, 20410, 20411, 20412, 20413, 20414, 20417, 20419, 20420, 20421, 25887,
        // decryptors — hacker loot
        23178, 23179, 23180, 23181, 23182, 21579, 21580, 21581, 21582, 21583,
        23183, 23184, 23185, 23186, 23187, 21573, 21574, 21575, 21576, 21577,
        // salvage (group 754) — hacker/ratter loot
        25588, 25589, 25590, 25591, 25593, 25594, 25599, 25605
    };
    const uint32 nGoods = sizeof(tradeGoods)/sizeof(tradeGoods[0]);

    // Try up to a few types: arbitrage the first crossing we find.
    uint32 startType = MakeRandomInt(0, (int32)nGoods - 1);
    bool didDeal = false;
    for (uint32 t = 0; t < nGoods && !didDeal; ++t) {
        uint32 typeID = tradeGoods[(startType + t) % nGoods];

        // Best resting ask and bid for this type at this station, from OTHER owners.
        DBQueryResult res;
        uint32 askOrderID = 0, bidOrderID = 0;
        double askPrice = 0, bidPrice = 0;
        uint32 askVol = 0, bidVol = 0;
        if (sDatabase.RunQuery(res,
            "SELECT orderID, price, volRemaining FROM mktOrders"
            " WHERE stationID = %u AND typeID = %u AND bid = 0 AND ownerID <> %u AND volRemaining > 0"
            " ORDER BY price ASC LIMIT 1", stationID, typeID, db.charID))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                askOrderID = row.GetUInt(0);
                askPrice = row.GetDouble(1);
                askVol = row.GetUInt(2);
            }
        }
        if (sDatabase.RunQuery(res,
            "SELECT orderID, price, volRemaining FROM mktOrders"
            " WHERE stationID = %u AND typeID = %u AND bid = 1 AND ownerID <> %u AND volRemaining > 0"
            " ORDER BY price DESC LIMIT 1", stationID, typeID, db.charID))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                bidOrderID = row.GetUInt(0);
                bidPrice = row.GetDouble(1);
                bidVol = row.GetUInt(2);
            }
        }

        // Do we have a real crossing (someone bids above someone else's ask)?
        // Only chase it if the spread is worth the bot's while.
        if (askOrderID != 0 && bidOrderID != 0 && bidPrice > askPrice) {
            // Confidence gates greed: a losing bot needs a WIDER edge to bother;
            // a winning one is happy with a thinner spread.
            double minSpread = (0.005 + (1.0 - confidence) * 0.05) * askPrice;  // 0.5%..5.5%
            if (bidPrice - askPrice >= minSpread) {
                // Volume scaled by confidence: bold bots move more.
                uint32 qty = 50 + (uint32)((confidence + 1.0f) * 0.5f * 950);   // 50..1000
                if (qty > askVol) qty = askVol;
                if (qty > bidVol) qty = bidVol;
                double profit = sMktMgr.BotArbitrageFill(db.charID, stationID, typeID,
                                                         askOrderID, bidOrderID, qty);
                if (profit > 0.0) {
                    mem.RecordTradeResult((int64)profit);
                    _log(BOT__TRACE, "BotMgr: trader %u arbitraged %u x type %u at %u, +%.0f ISK (conf %.2f).",
                         db.charID, qty, typeID, stationID, profit, confidence);
                }
                didDeal = true;   // tried one crossing — that's this cycle's action
            }
        }
    }

    // Market-making: if there was nothing to arbitrage, quote tighter than the
    // current book so the bot becomes the inside market and its order gets hit
    // by the next player/bot trade. Remembers its fills via tradeProfit too.
    if (!didDeal) {
        uint32 typeID = tradeGoods[(startType + MakeRandomInt(1, 3)) % nGoods];
        // Price anchor: what's the fair value? Prefer the live book midpoint,
        // fall back to the static base price (updated from live EVE by
        // import_prices.py) so bots don't quote nonsense far from reality.
        double fair = 0;
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT (SELECT MIN(price) FROM mktOrders WHERE stationID=%u AND typeID=%u AND bid=0 AND volRemaining>0 AND ownerID<>%u)"
            "     , (SELECT MAX(price) FROM mktOrders WHERE stationID=%u AND typeID=%u AND bid=1 AND volRemaining>0 AND ownerID<>%u)",
            stationID, typeID, db.charID, stationID, typeID, db.charID))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                double lo = row.GetDouble(0);
                double hi = row.GetDouble(1);
                if (lo > 0 && hi > 0)
                    fair = (lo + hi) / 2.0;
            }
        }
        if (fair <= 0) {
            const ItemType* type = sItemFactory.GetType(typeID);
            fair = type != nullptr ? type->basePrice() : 0;
        }
        if (fair <= 0)
            return;

        // Margin from self-learning: confident bots quote 0.5-1% inside; shaken
        // bots quote 3-6% outside so they don't get picked off repeatedly.
        double margin = (1.0 - confidence) * 0.04 + 0.006;    // 0.6%..4.6% per side
        double buyPrice  = fair * (1.0 - margin);
        double sellPrice = fair * (1.0 + margin);
        uint32 qty = 100 + (uint32)((confidence + 1.0f) * 0.5f * 1900);   // 100..2000

        // Only quote if we'd actually be inside the book (don't stack behind a
        // huge wall of stale NPC orders at a worse price).
        DBQueryResult qres;
        double bestAsk = 0, bestBid = 0;
        if (sDatabase.RunQuery(qres,
            "SELECT (SELECT MIN(price) FROM mktOrders WHERE stationID=%u AND typeID=%u AND bid=0 AND volRemaining>0 AND ownerID<>%u)"
            "     , (SELECT MAX(price) FROM mktOrders WHERE stationID=%u AND typeID=%u AND bid=1 AND volRemaining>0 AND ownerID<>%u)",
            stationID, typeID, db.charID, stationID, typeID, db.charID))
        {
            DBResultRow row;
            if (qres.GetRow(row)) {
                bestAsk = row.GetDouble(0);
                bestBid = row.GetDouble(1);
            }
        }
        // Under-cut the best ask / over-bid the best bid so we're the inside
        // quote; but never cross ourselves (sell <= buy would be nonsense).
        double placeSell = sellPrice, placeBuy = buyPrice;
        if (bestAsk > 0 && bestAsk < placeSell)
            placeSell = bestAsk * (1.0 - 0.004);     // 0.4% better than best ask
        if (bestBid > 0 && bestBid > placeBuy)
            placeBuy = bestBid * (1.0 + 0.004);      // 0.4% better than best bid
        if (placeSell <= placeBuy)
            return;   // crossing ourselves — skip, book is too thin to quote

        // Check the bot can actually fund a buy quote (escrow = price*qty).
        double balance = 0;
        DBQueryResult bres;
        if (sDatabase.RunQuery(bres, "SELECT balance FROM chrCharacters WHERE characterID = %u", db.charID)) {
            DBResultRow row;
            if (bres.GetRow(row))
                balance = row.GetDouble(0);
        }
        if (balance < placeBuy * qty)
            return;   // not enough ISK to back the quote — don't place air

        // Remove any stale quotes we left earlier at this station/type so the
        // book doesn't fill with old prices (re-quote instead of stacking).
        DBerror delErr;
        sDatabase.RunQuery(delErr, "DELETE FROM mktOrders WHERE ownerID = %u AND stationID = %u AND typeID = %u",
                           db.charID, stationID, typeID);

        DBerror err;
        sDatabase.RunQuery(err,
            "INSERT INTO mktOrders"
            "  (typeID, ownerID, regionID, stationID, solarSystemID, orderRange, bid, price,"
            "   escrow, minVolume, volEntered, volRemaining, issued, duration, isCorp, accountKey, memberID)"
            " VALUES"
            "  (%u, %u, (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u, 32767, 0, %.2f,"
            "   0, 1, %u, %u, %f, 90, 0, 1000, %u)",
            typeID, db.charID, sysID, stationID, sysID, placeSell, qty, qty, GetFileTimeNow(), db.charID);
        sDatabase.RunQuery(err,
            "INSERT INTO mktOrders"
            "  (typeID, ownerID, regionID, stationID, solarSystemID, orderRange, bid, price,"
            "   escrow, minVolume, volEntered, volRemaining, issued, duration, isCorp, accountKey, memberID)"
            " VALUES"
            "  (%u, %u, (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u, 32767, 1, %.2f,"
            "   %.2f, 1, %u, %u, %f, 90, 0, 1000, %u)",
            typeID, db.charID, sysID, stationID, sysID, placeBuy, placeBuy * qty, qty, qty, GetFileTimeNow(), db.charID);

        sMktMgr.InvalidateOrdersCache(sDataMgr.GetStationRegion(stationID), typeID, stationID);
        _log(BOT__TRACE, "BotMgr: trader %u quoting %u x type %u @%.2f/%.2f at %u (fair %.2f, conf %.2f).",
             db.charID, qty, typeID, placeBuy, placeSell, stationID, fair, confidence);
    }

    mem.Save();
}

// ============================================================================
// Industrialist (producer/builder)
//
// Runs a REAL multi-level manufacturing chain: the bill of materials is read
// from invTypeMaterials and resolved recursively (a T2 module needs components,
// which need minerals, ...). Missing inputs are bought from the best resting
// sell orders at the bot's station (MarketMgr::BotBuyStock mints them into the
// bot's hangar); crafted intermediates are consumed from the hangar and the
// finished product is minted back into it. The caller then ships/sells the
// output exactly like the other producers (courier to hub or direct sale).
//
// POS anchoring (tower/assembly array at a moon) and real PI colonies are the
// next stage; this function is the manufacturing core they will feed.
// ============================================================================

static uint32 BotInvQty(uint32 ownerID, uint32 locationID, uint32 flag, uint32 typeID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT COALESCE(SUM(quantity),0) FROM entity"
        " WHERE ownerID = %u AND locationID = %u AND flag = %u AND typeID = %u"
        "   AND singleton = 0 AND quantity > 0",
        ownerID, locationID, flag, typeID))
        return 0;
    DBResultRow row;
    if (res.GetRow(row)) return row.GetUInt(0);
    return 0;
}

static void BotInvConsume(uint32 ownerID, uint32 locationID, uint32 flag, uint32 typeID, uint32 qty)
{
    if (qty == 0) return;
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT itemID, quantity FROM entity"
        " WHERE ownerID = %u AND locationID = %u AND flag = %u AND typeID = %u"
        "   AND singleton = 0 AND quantity > 0 ORDER BY quantity ASC",
        ownerID, locationID, flag, typeID))
        return;
    DBResultRow row;
    uint32 need = qty;
    while (need > 0 && res.GetRow(row)) {
        uint32 itemID = row.GetUInt(0);
        uint32 have   = row.GetUInt(1);
        InventoryItemRef iRef = sItemFactory.GetItemRef(itemID);
        if (iRef.get() == nullptr)
            continue;
        if (have <= need) {
            need -= have;
            iRef->Delete();
        } else {
            iRef->SetQuantity((int32)(have - need), false);
            need = 0;
        }
    }
}

static bool BotInvMint(uint32 ownerID, uint32 locationID, uint32 flag, uint32 typeID, uint32 qty)
{
    if (qty == 0) return true;
    ItemData idata((uint16)typeID, ownerID, locationID, (EVEItemFlags)flag, qty);
    InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
    if (iRef.get() == nullptr) {
        _log(BOT__ERROR, "BotInvMint: failed to mint %u x type %u for %u.", qty, typeID, ownerID);
        return false;
    }
    iRef->SaveItem();
    return true;
}

static bool BotTypeHasMaterials(uint32 typeID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM invTypeMaterials WHERE typeID = %u", typeID)) {
        DBResultRow row;
        if (res.GetRow(row)) return row.GetUInt(0) > 0;
    }
    return false;
}

// Recursively ensure `runs` of typeID are in the given container (a station
// hangar or a POS module), crafting intermediates and buying base materials.
static bool BotCraftRecursive(uint32 ownerID, uint32 locationID, uint32 flag, uint32 typeID, uint32 runs, int depth)
{
    if (depth > 8)
        return false;
    if (runs == 0)
        return true;

    // Read the bill of materials for one run.
    struct Mat { uint32 typeID; uint32 qty; };
    std::vector<Mat> mats;
    {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT materialTypeID, quantity FROM invTypeMaterials WHERE typeID = %u", typeID))
            return false;
        DBResultRow row;
        while (res.GetRow(row)) {
            Mat m; m.typeID = row.GetUInt(0); m.qty = row.GetUInt(1);
            if (m.typeID != 0 && m.qty != 0) mats.push_back(m);
        }
    }

    // Leaf (no recipe): must be bought from the market (local station).
    if (mats.empty()) {
        uint32 have = BotInvQty(ownerID, locationID, flag, typeID);
        if (have < runs) {
            sMktMgr.BotBuyStock(ownerID, locationID, typeID, runs - have);
            have = BotInvQty(ownerID, locationID, flag, typeID);
            if (have < runs)
                sMktMgr.BotBuyStockRemote(ownerID, locationID, typeID, runs - have);  // import from the region
        }
        return BotInvQty(ownerID, locationID, flag, typeID) >= runs;
    }

    for (const auto& m : mats) {
        uint32 needTotal = m.qty * runs;
        uint32 have = BotInvQty(ownerID, locationID, flag, m.typeID);
        if (have < needTotal) {
            uint32 missing = needTotal - have;
            if (BotTypeHasMaterials(m.typeID)) {
                if (!BotCraftRecursive(ownerID, locationID, flag, m.typeID, missing, depth + 1))
                    return false;
            } else {
                sMktMgr.BotBuyStock(ownerID, locationID, m.typeID, missing);
                uint32 nowHave = BotInvQty(ownerID, locationID, flag, m.typeID);
                if (nowHave < needTotal)
                    sMktMgr.BotBuyStockRemote(ownerID, locationID, m.typeID, needTotal - nowHave); // import
            }
            if (BotInvQty(ownerID, locationID, flag, m.typeID) < needTotal)
                return false;   // market couldn't supply the input
        }
        BotInvConsume(ownerID, locationID, flag, m.typeID, needTotal);
    }

    return BotInvMint(ownerID, locationID, flag, typeID, runs);
}

// ---- planetary industry: real schematic chain (P1 -> P2 -> P3 -> P4) --------
static uint32 BotTypeGroup(uint32 typeID)
{
    DBQueryResult res;
    if (sDatabase.RunQuery(res, "SELECT groupID FROM invTypes WHERE typeID = %u", typeID)) {
        DBResultRow row;
        if (res.GetRow(row)) return row.GetUInt(0);
    }
    return 0;
}

static uint32 BotSchematicForOutput(uint32 typeID)
{
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT schematicID FROM schematicsTypeMap WHERE typeID = %u AND isInput = 0 LIMIT 1", typeID)) {
        DBResultRow row;
        if (res.GetRow(row)) return row.GetUInt(0);
    }
    return 0;
}

// Produce `qty` of a PI commodity: P1 (Basic) is EXTRACTED (minted free by the
// colony), higher tiers are refined via their real schematic (inputs produced
// recursively). Non-PI leaves fall back to the market.
static bool BotCraftPI(uint32 ownerID, uint32 locationID, uint32 flag, uint32 typeID, uint32 qty, int depth)
{
    if (depth > 6 || qty == 0)
        return true;

    if (BotTypeGroup(typeID) == EVEDB::invGroups::Basic_Commodities) {
        return BotInvMint(ownerID, locationID, flag, typeID, qty);   // extraction
    }

    uint32 sch = BotSchematicForOutput(typeID);
    if (sch == 0) {
        uint32 have = BotInvQty(ownerID, locationID, flag, typeID);
        if (have < qty) {
            sMktMgr.BotBuyStock(ownerID, locationID, typeID, qty - have);
            have = BotInvQty(ownerID, locationID, flag, typeID);
            if (have < qty)
                sMktMgr.BotBuyStockRemote(ownerID, locationID, typeID, qty - have);
        }
        return BotInvQty(ownerID, locationID, flag, typeID) >= qty;
    }

    // output quantity per cycle
    uint32 outQty = 1;
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT quantity FROM schematicsTypeMap WHERE schematicID = %u AND typeID = %u AND isInput = 0 LIMIT 1",
            sch, typeID)) {
            DBResultRow row;
            if (res.GetRow(row)) outQty = row.GetUInt(0) ? row.GetUInt(0) : 1;
        }
    }
    uint32 cycles = (qty + outQty - 1) / outQty;

    // consume inputs (produce recursively first)
    DBQueryResult ir;
    if (!sDatabase.RunQuery(ir,
        "SELECT typeID, quantity FROM schematicsTypeMap WHERE schematicID = %u AND isInput = 1", sch))
        return false;
    std::vector<std::pair<uint32,uint32>> ins;
    {
        DBResultRow r;
        while (ir.GetRow(r)) ins.push_back({ r.GetUInt(0), r.GetUInt(1) });
    }
    for (auto& in : ins) {
        uint32 need = in.second * cycles;
        uint32 have = BotInvQty(ownerID, locationID, flag, in.first);
        if (have < need) {
            if (!BotCraftPI(ownerID, locationID, flag, in.first, need - have, depth + 1))
                return false;
        }
        if (BotInvQty(ownerID, locationID, flag, in.first) < need)
            return false;
        BotInvConsume(ownerID, locationID, flag, in.first, need);
    }

    return BotInvMint(ownerID, locationID, flag, typeID, cycles * outQty);
}

void BotMgr::DeployBotPOS(SystemManager* sysMgr, uint32 charID, uint32 corpID)
{
    if (sysMgr == nullptr || charID == 0 || corpID == 0)
        return;
    uint32 sysID = sysMgr->GetID();

    // Already a tower for this corp here? top up its fuel (and re-online it if it
    // ran dry) instead of deploying a duplicate. NOTE: entity has no groupID
    // column — join invTypes (the old query used a non-existent column and the
    // check never matched, so a new tower was spawned on every docked cycle).
    {
        DBQueryResult chk;
        if (sDatabase.RunQuery(chk,
            "SELECT e.itemID FROM entity e JOIN invTypes t ON t.typeID = e.typeID"
            " WHERE e.locationID = %u AND e.ownerID = %u AND t.groupID = %u LIMIT 1",
            sysID, corpID, EVEDB::invGroups::Control_Tower)) {
            DBResultRow r;
            if (chk.GetRow(r)) {
                SystemEntity* existing = sysMgr->GetSE(r.GetUInt(0));
                if (existing != nullptr && existing->GetTowerSE() != nullptr)
                    existing->GetTowerSE()->BotEnsureFuel(720);
                return;
            }
        }
    }

    // Pick a moon in this system (groupID 8 = Moon in mapDenormalize).
    GPoint moonPos;
    {
        DBQueryResult mres;
        if (!sDatabase.RunQuery(mres,
            "SELECT x, y, z FROM mapDenormalize WHERE solarSystemID = %u AND groupID = 8 ORDER BY RAND() LIMIT 1",
            sysID))
            return;
        DBResultRow mrow;
        if (!mres.GetRow(mrow))
            return;
        moonPos.x = mrow.GetDouble(0);
        moonPos.y = mrow.GetDouble(1);
        moonPos.z = mrow.GetDouble(2);
    }
    // Anchor ~80-120 km off the moon (EVE moonAnchorDistance).
    GPoint pos = moonPos;
    pos.x += 80000.0 + MakeRandomInt(0, 40000);

    uint32 towerType = 0, arrayType = 0, siloType = 0;
    {
        DBQueryResult tres;
        if (sDatabase.RunQuery(tres,
            "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 ORDER BY RAND() LIMIT 1",
            EVEDB::invGroups::Control_Tower)) {
            DBResultRow r; if (tres.GetRow(r)) towerType = r.GetUInt(0);
        }
    }
    if (towerType == 0)
        return;
    {
        DBQueryResult ares;
        if (sDatabase.RunQuery(ares,
            "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 ORDER BY RAND() LIMIT 1",
            EVEDB::invGroups::Assembly_Array)) {
            DBResultRow r; if (ares.GetRow(r)) arrayType = r.GetUInt(0);
        }
        DBQueryResult sres;
        if (sDatabase.RunQuery(sres,
            "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 ORDER BY RAND() LIMIT 1",
            EVEDB::invGroups::Silo)) {
            DBResultRow r; if (sres.GetRow(r)) siloType = r.GetUInt(0);
        }
    }

    FactionData data = FactionData();
        data.ownerID = charID;
        data.corporationID = corpID;
        data.allianceID = 0;
        data.factionID = 0;

    // Spawn the tower first (modules need the tower in the bubble).
    double ffRadius = 20000.0;   // tower force-field radius — module anchor limit
    {
        ItemData idata(towerType, corpID, sysID, flagNone, "Control Tower", pos);
        StructureItemRef sRef = sItemFactory.SpawnStructure(idata);
        if (sRef.get() == nullptr) {
            _log(BOT__ERROR, "DeployBotPOS: failed to spawn tower type %u for corp %u.", towerType, corpID);
            return;
        }
        if (sRef->HasAttribute(AttrShieldRadius)) {
            double r = sRef->GetAttribute(AttrShieldRadius).get_float();
            if (r > 5000.0)
                ffRadius = r;
        }
        // Mark it as a bot POS so TowerSE fuels/re-onlines it on load (Process).
        sRef->SetCustomInfo("botpos");
        sRef->SaveItem();
        TowerSE* tSE = new TowerSE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
        sysMgr->AddEntity(tSE);
        tSE->BotDeployAndAnchor(pos);
    }

    // --- POS layout ---------------------------------------------------------
    // Every module must sit INSIDE the tower's force field (an inscribed circle)
    // or the client treats it as unanchored. Production (array + silo) is
    // clustered within 2500 m of each other just off the tower; the batteries are
    // spread out — top, bottom and on the ring — for all-round coverage.
    const double R = ffRadius;

    auto spawnModule = [&](uint32 typeID, const char* name, const GPoint& off) {
        if (typeID == 0) return;
        GPoint p = pos; p.x += off.x; p.y += off.y; p.z += off.z;
        ItemData idata(typeID, corpID, sysID, flagNone, name, p);
        StructureItemRef sRef = sItemFactory.SpawnStructure(idata);
        if (sRef.get() == nullptr) return;
        sRef->SaveItem();
        // Assembly arrays and silos get their proper SE classes.
        uint16 gid = sRef->groupID();
        StructureSE* se = nullptr;
        if (gid == EVEDB::invGroups::Assembly_Array)
            se = new ArraySE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
        else
            se = new ReactorSE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
        sysMgr->AddEntity(se);
        se->BotDeployAndAnchor(p);
    };

    // Production cluster ~2.2 km off the tower; array and silo 1.8 km apart
    // (well within the 2500 m the user asked for).
    spawnModule(arrayType, "Assembly Array", GPoint(2200.0, 0.0,  900.0));
    spawnModule(siloType,  "Silo",           GPoint(2200.0, 0.0, -900.0));

    // Standard POS defenses: WEAPON batteries (WeaponSE owns a POS_AI, which is
    // implemented and now fires at valid hostiles). Anchored online so the tower
    // is genuinely defended, not just decorated.
    uint32 weaponType1 = 0, weaponType2 = 0;
    {
        DBQueryResult wres;
        if (sDatabase.RunQuery(wres,
            "SELECT typeID FROM invTypes WHERE groupID IN (430,426,417,449) AND published = 1"
            " ORDER BY RAND() LIMIT 2")) {
            DBResultRow wr;
            if (wres.GetRow(wr)) weaponType1 = wr.GetUInt(0);
            if (wres.GetRow(wr)) weaponType2 = wr.GetUInt(0);
        }
    }
    auto spawnWeapon = [&](uint32 typeID, const GPoint& off) {
        if (typeID == 0) return;
        GPoint p = pos; p.x += off.x; p.y += off.y; p.z += off.z;
        ItemData idata(typeID, corpID, sysID, flagNone, "Weapon Battery", p);
        StructureItemRef sRef = sItemFactory.SpawnStructure(idata);
        if (sRef.get() == nullptr) return;
        sRef->SaveItem();
        // Load the gun: mint a stack of its chargeGroup1 ammunition into the
        // module's own hold (consumed 1 per shot by POS_AI).
        if (sRef->HasAttribute(AttrChargeGroup1)) {
            uint32 cg = sRef->GetAttribute(AttrChargeGroup1).get_uint32();
            if (cg != 0) {
                DBQueryResult cr;
                uint32 chargeType = 0;
                if (sDatabase.RunQuery(cr,
                    "SELECT typeID FROM invTypes WHERE groupID = %u AND published = 1 ORDER BY RAND() LIMIT 1", cg)) {
                    DBResultRow cRow;
                    if (cr.GetRow(cRow)) chargeType = cRow.GetUInt(0);
                }
                if (chargeType != 0) {
                    ItemData cdata((uint16)chargeType, corpID, sRef->itemID(), flagNone, 5000);
                    InventoryItemRef cRef = sItemFactory.SpawnItem(cdata);
                    if (cRef.get() != nullptr)
                        cRef->SaveItem();
                }
            }
        }
        WeaponSE* se = new WeaponSE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
        sysMgr->AddEntity(se);
        se->BotDeployAndAnchor(p);
    };

    // Defenses spread for all-round coverage: one gun high (+Y), one low (-Y),
    // and the EWAR battery out on the ring (+X) — all inside the force field.
    spawnWeapon(weaponType1, GPoint(0.0,  0.60 * R, 0.0));   // top
    spawnWeapon(weaponType2, GPoint(0.0, -0.60 * R, 0.0));   // bottom

    // EWAR defense: a stasis webification battery (BatterySE now runs POS_AI too).
    {
        GPoint p = pos; p.x += 0.70 * R;   // on the force-field ring
        ItemData idata(17178, corpID, sysID, flagNone, "Stasis Webification Battery", p);
        StructureItemRef sRef = sItemFactory.SpawnStructure(idata);
        if (sRef.get() != nullptr) {
            sRef->SaveItem();
            BatterySE* se = new BatterySE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
            sysMgr->AddEntity(se);
            se->BotDeployAndAnchor(p);
        }
    }

    // Cost of the installation (tower + modules + defenses + initial guards'
    // retainer), debited from the owner's wallet so a POS is an investment
    // comparable to the value of what it produces.
    double cost = 0.0;
    {
        DBQueryResult cres;
        if (sDatabase.RunQuery(cres,
            "SELECT COALESCE(SUM(basePrice),0) FROM invTypes WHERE typeID IN (%u,%u,%u,%u,%u)",
            towerType, arrayType ? arrayType : towerType, siloType ? siloType : towerType,
            weaponType1 ? weaponType1 : towerType, weaponType2 ? weaponType2 : towerType)) {
            DBResultRow cr;
            if (cres.GetRow(cr)) cost = cr.GetDouble(0);
        }
    }
    cost = cost * 1.5 + 5000000.0;   // + fit/defense/guards retainer
    if (cost > 0.0) {
        DBerror cerr;
        sDatabase.RunQuery(cerr,
            "UPDATE chrCharacters SET balance = GREATEST(0, balance - %.2f) WHERE characterID = %u", cost, charID);
        _log(BOT__MESSAGE, "BotMgr: POS installation cost %.0f ISK charged to %u.", cost, charID);
    }

    _log(BOT__MESSAGE, "BotMgr: industrialist %u deployed a POS at a moon in system %u (tower %u, array %u, silo %u).",
         charID, sysID, towerType, arrayType, siloType);

    // Corp guards: defend the tower (2-3 pilots of the owner corp).
    SpawnPosGuards(sysMgr, corpID, pos);
}

void BotMgr::SpawnPosGuards(SystemManager* sysMgr, uint32 corpID, const GPoint& pos)
{
    if (sysMgr == nullptr || corpID == 0)
        return;
    uint32 sysID = sysMgr->GetID();
    float sec = sysMgr->GetSystemSecurityRating();

    // Find same-corp pool pilots to man the tower.
    std::vector<std::pair<uint32,std::string>> candidates;
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT c.characterID, c.characterName FROM chrCharacters c"
            " JOIN botMemory b ON b.charID = c.characterID"
            " WHERE c.accountID = 0 AND c.corporationID = %u AND c.characterName != ''"
            " ORDER BY RAND() LIMIT 6", corpID)) {
            DBResultRow r;
            while (res.GetRow(r))
                candidates.push_back({ r.GetUInt(0), r.GetText(1) });
        }
    }
    if (candidates.empty())
        return;

    int want = 2 + MakeRandomInt(0, 1);   // 2-3 guards
    int spawned = 0;
    for (auto& cand : candidates) {
        if (spawned >= want)
            break;
        uint32 charID = cand.first;

        // Skip if this pilot is already flying in the system.
        bool present = false;
        for (auto& [eid, se] : sysMgr->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr) continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr && pb->GetBotCharID() == charID) { present = true; break; }
        }
        if (present)
            continue;

        SpawnBot(sysMgr, charID, cand.second, corpID, 0);

        // Locate the freshly spawned guard and turn it into a tower defender.
        PlayerBot* guard = nullptr;
        for (auto& [eid, se] : sysMgr->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr) continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr && pb->GetBotCharID() == charID) { guard = pb; break; }
        }
        if (guard == nullptr)
            continue;

        guard->SetProfession(PlayerBot::BotProfession::Hunter);
        guard->SetPosGuard(true);

        // Arrival model: null-sec mostly "login at the POS"; high-sec 50/50.
        bool loginAtPos = (sec < 0.5f) ? true : (MakeRandomInt(0, 1) == 1);

        // Find the tower to orbit.
        SystemEntity* tower = nullptr;
        for (auto& [eid, se] : sysMgr->GetEntities()) {
            if (se != nullptr && se->GetTowerSE() != nullptr) { tower = se; break; }
        }

        if (loginAtPos) {
            // "Login warp": the pilot appears at the POS (out of nowhere).
            GPoint p = pos;
            p.x += MakeRandomInt(-4000, 4000);
            p.y += MakeRandomInt(-4000, 4000);
            guard->DestinyMgr()->SetPosition(p);
        } else if (tower != nullptr) {
            // "Login at station, then warp in": visible warp to the tower.
            guard->DestinyMgr()->WarpTo(pos, 0);
        }

        // Hold station on the tower (guards don't wander off).
        if (tower != nullptr && guard->DestinyMgr() != nullptr)
            guard->DestinyMgr()->Orbit(tower, 5000 + MakeRandomInt(0, 3000));

        _log(BOT__MESSAGE, "BotMgr: POS guard %s(%u) assigned to tower in system %u (%s arrival).",
             cand.second.c_str(), charID, sysID, loginAtPos ? "login-at-POS" : "station-warp");
        ++spawned;
    }
}

void BotMgr::ProcessPosGuards()
{
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;

    for (auto& [sysID, pSystem] : sEntityList.GetSystems()) {
        if (pSystem == nullptr)
            continue;

        // Find the tower (if any) in this system.
        SystemEntity* tower = nullptr;
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se != nullptr && se->GetTowerSE() != nullptr) { tower = se; break; }
        }
        if (tower == nullptr)
            continue;

        uint32 manual = tower->GetTowerSE()->GetManualTarget();
        if (manual == 0)
            continue;   // no operator target — guards rely on their own Hunter AI

        SystemEntity* targ = pSystem->GetSE(manual);
        if (targ == nullptr)
            continue;

        // Guards focus the operator's target too (helps kill it fast).
        for (auto& [id, se] : pSystem->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb == nullptr || !pb->IsPosGuard())
                continue;
            if (pb->GetAIMgr() != nullptr && !pb->GetAIMgr()->IsFighting())
                pb->GetAIMgr()->Target(targ);
        }
    }
}

void BotMgr::ProcessDockedIndustrialEconomy(uint32 sysID, uint32 stationID, const DockedBot& db)
{
    if (sysID == 0 || stationID == 0 || db.charID == 0)
        return;

    // In empire (highsec) the producer anchors its own POS at a moon, then runs
    // the manufacturing chain there. (In nullsec this will move to claimed
    // systems with bridge logistics; for now only highsec stations have bots.)
    SystemManager* sMgr = sEntityList.FindOrBootSystem(sysID);
    if (sMgr != nullptr && sMgr->GetSystemSecurityRating() >= 0.5f && db.corpID != 0) {
        // The deploy itself is idempotent (one tower per corp/system), so try on
        // every docked cycle until the corp has its POS.
        DeployBotPOS(sMgr, db.charID, db.corpID);
    }

    // Pick a random T1 product we can actually build (module/charge/ship), cheap
    // enough for a bot wallet. T2 lines are reachable through recursion once the
    // T1/components are available on the local market.
    uint32 productID = 0, productCat = 0;
    DBQueryResult pres;
    if (sDatabase.RunQuery(pres,
        "SELECT bp.productTypeID, g.categoryID FROM invBlueprintTypes bp"
        " JOIN invTypes t ON t.typeID = bp.productTypeID"
        " JOIN invGroups g ON g.groupID = t.groupID"
        " JOIN invCategories c ON c.categoryID = g.categoryID"
        " WHERE bp.techLevel = 1 AND t.published = 1"
        "   AND c.categoryID IN (6, 7, 8)"            // Ship, Module, Charge
        "   AND t.basePrice > 0 AND t.basePrice < 20000000"
        " ORDER BY RAND() LIMIT 1"))
    {
        DBResultRow prow;
        if (pres.GetRow(prow)) { productID = prow.GetUInt(0); productCat = prow.GetUInt(1); }
    }
    if (productID == 0)
        return;

    uint32 runs = (productCat == 8) ? 100 : 1;   // ammo/charges in batches

    // Craft location: the POS Assembly Array when the corp has one in this
    // system (physical production at the tower), otherwise the station hangar.
    uint32 craftLoc = stationID;
    bool craftAtPOS = false;
    if (db.corpID != 0) {
        DBQueryResult pa;
        if (sDatabase.RunQuery(pa,
            "SELECT itemID FROM entity WHERE ownerID = %u AND locationID = %u AND groupID = %u LIMIT 1",
            db.corpID, sysID, EVEDB::invGroups::Assembly_Array)) {
            DBResultRow par;
            if (pa.GetRow(par)) { craftLoc = par.GetUInt(0); craftAtPOS = true; }
        }
    }

    if (BotCraftRecursive(db.charID, craftLoc, (uint32)flagHangar, productID, runs, 0)) {
        _log(BOT__MESSAGE, "BotMgr: industrialist %s(%u) built %u x %s %s.",
             db.name.c_str(), db.charID, runs, sDataMgr.GetTypeName(productID),
             craftAtPOS ? "at its POS" : "at the station");

        // Physical production at the POS: haul the finished goods back to the
        // station hangar so the normal courier/sale logistics can move them.
        if (craftAtPOS) {
            DBQueryResult mv;
            if (sDatabase.RunQuery(mv,
                "SELECT itemID FROM entity WHERE ownerID = %u AND locationID = %u AND flag = %u",
                db.charID, craftLoc, (uint32)flagHangar)) {
                DBResultRow mvr;
                while (mv.GetRow(mvr)) {
                    InventoryItemRef iRef = sItemFactory.GetItemRef(mvr.GetUInt(0));
                    if (iRef.get() != nullptr)
                        iRef->Move(stationID, flagHangar);
                }
            }
        }
    } else {
        _log(BOT__TRACE, "BotMgr: industrialist %s(%u) could not source materials for %s — skipping.",
             db.name.c_str(), db.charID, sDataMgr.GetTypeName(productID));
    }

    // Planetary industry: run the bot's colony schematic chain (P1 extracted,
    // refined up to P2/P3/P4) and deliver the output into the hangar.
    ProcessIndustrialistPI(sysID, stationID, db);
}

void BotMgr::DeployBotCustomsOffice(SystemManager* sysMgr, uint32 charID, uint32 corpID, uint32 planetID)
{
    if (sysMgr == nullptr || corpID == 0 || planetID == 0)
        return;
    uint32 sysID = sysMgr->GetID();

    // Idempotent: one corp office per planet.
    {
        DBQueryResult chk;
        if (sDatabase.RunQuery(chk,
            "SELECT COUNT(*) FROM entity WHERE locationID = %u AND ownerID = %u AND typeID = 2233 AND customInfo = '%u'",
            sysID, corpID, planetID)) {
            DBResultRow r;
            if (chk.GetRow(r) && r.GetUInt(0) > 0)
                return;
        }
    }

    GPoint pos;
    {
        DBQueryResult pr;
        if (!sDatabase.RunQuery(pr,
            "SELECT x, y, z, IFNULL(radius,0) FROM mapDenormalize WHERE itemID = %u", planetID))
            return;
        DBResultRow r;
        if (!pr.GetRow(r))
            return;
        pos.x = r.GetDouble(0) + r.GetDouble(3) + 50000.0;
        pos.y = r.GetDouble(1);
        pos.z = r.GetDouble(2);
    }

    ItemData idata(2233, corpID, sysID, flagNone, "Customs Office", pos);
    StructureItemRef sRef = sItemFactory.SpawnStructure(idata);
    if (sRef.get() == nullptr) {
        _log(BOT__ERROR, "DeployBotCustomsOffice: failed to spawn office for corp %u at planet %u.", corpID, planetID);
        return;
    }
    sRef->SetCustomInfo(std::to_string(planetID).c_str());
    sRef->SaveItem();

    FactionData data = FactionData();
        data.ownerID = corpID;
        data.corporationID = corpID;
        data.allianceID = 0;
        data.factionID = 0;

    CustomsSE* se = new CustomsSE(sRef, sysMgr->GetServiceMgr(), sysMgr, data);
    sysMgr->AddEntity(se);   // Init() binds the planet from customInfo and marks it online
    _log(BOT__MESSAGE, "BotMgr: industrialist %u anchored a corp Customs Office at planet %u (corp %u).",
         charID, planetID, corpID);
}

void BotMgr::ProcessIndustrialistPI(uint32 sysID, uint32 stationID, const DockedBot& db)
{
    if (db.charID == 0 || sysID == 0)
        return;

    uint32 planetID = 0, sch = 0;
    int64 lastRun = 0;
    bool have = false;
    {
        DBQueryResult cr;
        if (sDatabase.RunQuery(cr,
            "SELECT planetID, schematicID, lastRun FROM botColonies WHERE charID = %u", db.charID)) {
            DBResultRow r;
            if (cr.GetRow(r)) {
                planetID = r.GetUInt(0); sch = r.GetUInt(1); lastRun = r.GetInt64(2);
                have = true;
            }
        }
    }

    if (!have) {
        // Pick a planet and a self-contained schematic (all inputs are PI
        // commodities, so the colony needs nothing but its own extraction).
        DBQueryResult pr;
        if (sDatabase.RunQuery(pr,
            "SELECT itemID FROM mapDenormalize WHERE solarSystemID = %u AND groupID = 7 ORDER BY RAND() LIMIT 1",
            sysID)) {
            DBResultRow r; if (pr.GetRow(r)) planetID = r.GetUInt(0);
        }
        if (planetID == 0)
            return;
        DBQueryResult sr;
        if (sDatabase.RunQuery(sr,
            "SELECT s.schematicID FROM schematics s"
            " JOIN schematicsTypeMap stm ON stm.schematicID = s.schematicID AND stm.isInput = 0"
            " JOIN invTypes t ON t.typeID = stm.typeID"
            " WHERE t.groupID IN (1034,1040,1041)"
            "   AND s.schematicID NOT IN ("
            "       SELECT stm2.schematicID FROM schematicsTypeMap stm2"
            "       JOIN invTypes t2 ON t2.typeID = stm2.typeID"
            "       WHERE stm2.isInput = 1 AND t2.groupID NOT IN (1042,1034,1040,1041))"
            " ORDER BY RAND() LIMIT 1")) {
            DBResultRow r; if (sr.GetRow(r)) sch = r.GetUInt(0);
        }
        if (sch == 0)
            return;
        lastRun = GetFileTimeNow();
        DBerror err;
        sDatabase.RunQuery(err,
            "INSERT INTO botColonies (charID, planetID, schematicID, lastRun) VALUES (%u, %u, %u, %lli)"
            " ON DUPLICATE KEY UPDATE planetID=VALUES(planetID), schematicID=VALUES(schematicID), lastRun=VALUES(lastRun)",
            db.charID, planetID, sch, lastRun);
        _log(BOT__MESSAGE, "BotMgr: industrialist %s(%u) established a PI colony on planet %u (schematic %u).",
             db.name.c_str(), db.charID, planetID, sch);
        return;
    }

    if (sch == 0)
        return;

    // Corp customs office on the colony planet (PI export/import structure).
    if (db.corpID != 0 && planetID != 0) {
        SystemManager* sMgr = sEntityList.FindOrBootSystem(sysID);
        if (sMgr != nullptr)
            DeployBotCustomsOffice(sMgr, db.charID, db.corpID, planetID);
    }

    uint32 cycleTime = 1800, outType = 0, outQty = 1;
    {
        DBQueryResult r1;
        if (sDatabase.RunQuery(r1, "SELECT cycleTime FROM schematics WHERE schematicID = %u", sch)) {
            DBResultRow r; if (r1.GetRow(r)) cycleTime = r.GetUInt(0) ? r.GetUInt(0) : 1800;
        }
        DBQueryResult r2;
        if (sDatabase.RunQuery(r2,
            "SELECT typeID, quantity FROM schematicsTypeMap WHERE schematicID = %u AND isInput = 0 LIMIT 1", sch)) {
            DBResultRow r;
            if (r2.GetRow(r)) { outType = r.GetUInt(0); outQty = r.GetUInt(1) ? r.GetUInt(1) : 1; }
        }
    }
    if (outType == 0)
        return;

    int64 now = GetFileTimeNow();
    int64 per = (int64)cycleTime * 10000000LL;   // seconds -> filetime (100ns)
    if (per <= 0) per = 1800LL * 10000000LL;
    uint32 cycles = (uint32)((now - lastRun) / per);
    if (cycles == 0)
        return;
    if (cycles > 20) cycles = 20;

    if (BotCraftPI(db.charID, stationID, (uint32)flagHangar, outType, cycles * outQty, 0)) {
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE botColonies SET lastRun = %lli WHERE charID = %u", now, db.charID);
        _log(BOT__MESSAGE, "BotMgr: industrialist %s(%u) ran its PI colony (%u cycles): %u x %s.",
             db.name.c_str(), db.charID, cycles, cycles * outQty, sDataMgr.GetTypeName(outType));
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

void BotMgr::PayMissionReward(PlayerBot* bot)
{
    // Agent mission payout: a missioner is credited ISK when it docks and reports
    // in after a run (like turning a mission in to an agent). Scaled a little by
    // how much practice the pilot has, so veterans earn more.
    uint32 charID = bot->GetBotCharID();
    if (charID == 0)
        return;
    double reward = 30000.0 + MakeRandomInt(0, 80000);   // 30k-110k per report
    if (bot->GetMemory() != nullptr)
        reward *= 1.0 + 0.25 * bot->GetMemory()->GetActivitySkill();   // up to +25%
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET balance = balance + %f WHERE characterID = %u",
                       reward, charID);
    _log(BOT__MESSAGE, "BotMgr: missioner %s(%u) reported in — %.0f ISK mission payout.",
         bot->GetBotName().c_str(), charID, reward);
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
        "   dateIssued, dateExpired, expireTimeInMinutes, duration, numDays, startStationID, startSolarSystemID,"
        "   startRegionID, endStationID, endSolarSystemID, endRegionID, price, reward, collateral,"
        "   title, description, status, volume, startStationDivision)"
        " VALUES"
        "  (3, %u, %u, 0, 0, 0, %lli, %lli, 10080, 7, 7, %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), 0, %lli, 0,"
        "   'Courier shipment', 'Standard courier contract', 0, %f, 1000)",
        charID, corpID,
        (int64)GetFileTimeNow(), (int64)GetFileTimeNow() + 7LL * EvE::Time::Day,
        startStation, sysID, sysID, endStation, endSys, endSys, reward, volume))
    {
        _log(BOT__MESSAGE, "BotMgr: trader %u issued courier contract (%.0f m3, reward %.0f ISK) %u -> %u.",
             charID, volume, (double)reward, sysID, endSys);
    }
}

// Stage-2 physical-goods haul: when a bot has real stock sitting in its station
// hangar (ore / minerals / salvage / faction loot deposited by miners and rat
// hunters), it packs that cargo into a PUBLIC courier contract to the trade hub.
// The real items are locked into the contract (owner -> contract), so a courier
// bot (or player) can accept and haul them; on completion CompleteContract moves
// them into the issuer's hangar at the hub. Goods physically travel between
// stations — the "living economy" haul step. Returns the new contract id (0 if
// there was nothing worth shipping).
uint32 BotMgr::PlaceStockCourierContractAt(uint32 sysID, uint32 stationID, uint32 charID, uint32 corpID)
{
    if (sysID == 0 || stationID == 0 || charID == 0)
        return 0;

    // Pick the cargo: real stock owned by this bot in this station's hangar.
    // Prefer non-ship stacks (type volume sane); cap the shipment at ~ a
    // freighter hold so contracts stay reasonable.
    struct StockType { uint16 typeID; uint32 qty; float vol; };
    std::vector<StockType> stock;
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT typeID, SUM(quantity) FROM entity"
        " WHERE ownerID = %u AND locationID = %u AND flag = %u AND quantity > 0 AND singleton = 0"
        " GROUP BY typeID", charID, stationID, (uint32)flagHangar))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            StockType st;
            st.typeID = (uint16)row.GetUInt(0);
            st.qty = row.GetUInt(1);
            const ItemType* t = sItemFactory.GetType(st.typeID);
            st.vol = t != nullptr ? t->volume() : 1.0f;
            if (st.vol < 0.01f) st.vol = 1.0f;
            stock.push_back(st);
        }
    }
    if (stock.empty())
        return 0;

    // Keep it to a courier-size load (~15k m3, a few distinct types).
    const double maxVol = 15000.0;
    double totalVol = 0;
    uint32 cargoTypes = 0;
    std::vector<StockType> cargo;
    for (const auto& st : stock) {
        if (cargoTypes >= 6)
            break;
        if (totalVol + st.vol * st.qty > maxVol) {
            // Ship what fits of this type.
            uint32 fit = (uint32)((maxVol - totalVol) / st.vol);
            if (fit > 0) {
                StockType part = st; part.qty = fit;
                cargo.push_back(part);
                totalVol += st.vol * fit;
                ++cargoTypes;
            }
            break;
        }
        cargo.push_back(st);
        totalVol += st.vol * st.qty;
        ++cargoTypes;
    }
    if (cargo.empty() || totalVol < 500.0)   // not worth a courier's time
        return 0;

    // Destination: the primary trade hub (Jita) so goods end up where the best
    // prices are. If we're already at the hub, pick another random station.
    uint32 endSys = GetTradeHubSystem();
    uint32 endStation = 0;
    if (endSys == 0 || endSys == sysID) {
        DBQueryResult rres;
        if (!sDatabase.RunQuery(rres, "SELECT stationID, solarSystemID FROM staStations ORDER BY RAND() LIMIT 1"))
            return 0;
        DBResultRow rrow;
        if (!rres.GetRow(rrow))
            return 0;
        endStation = rrow.GetUInt(0);
        endSys = rrow.GetUInt(1);
        if (endStation == stationID)
            return 0;
    } else {
        DBQueryResult rres;
        if (!sDatabase.RunQuery(rres, "SELECT stationID FROM staStations WHERE solarSystemID = %u LIMIT 1", endSys))
            return 0;
        DBResultRow rrow;
        if (!rres.GetRow(rrow))
            return 0;
        endStation = rrow.GetUInt(0);
    }
    if (endStation == 0 || endStation == stationID)
        return 0;

    // Reward scales with volume so hauling is worth a courier's time.
    int64 reward = (int64)(40000 + totalVol * 30.0);

    DBerror err;
    uint32 contractId = 0;
    if (!sDatabase.RunQueryLID(err, contractId,
        "INSERT INTO ctrContracts"
        "  (contractType, issuerID, issuerCorpID, forCorp, isPrivate, assigneeID,"
        "   dateIssued, dateExpired, expireTimeInMinutes, duration, numDays, startStationID, startSolarSystemID,"
        "   startRegionID, endStationID, endSolarSystemID, endRegionID, price, reward, collateral,"
        "   title, description, status, volume, startStationDivision)"
        " VALUES"
        "  (3, %u, %u, 0, 0, 0, %lli, %lli, 10080, 7, 7, %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), %u, %u,"
        "   (SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u), 0, %lli, 0,"
        "   'Mineral shipment', 'Bulk commodity haul', 0, %f, 1000)",
        charID, corpID,
        (int64)GetFileTimeNow(), (int64)GetFileTimeNow() + 7LL * EvE::Time::Day,
        stationID, sysID, sysID, endStation, endSys, endSys, reward, totalVol))
    {
        _log(BOT__ERROR, "PlaceStockCourierContractAt: failed to insert contract for %u.", charID);
        return 0;
    }

    // Lock the physical cargo into the contract and record it in ctrItems.
    // Owner -> 1 (contract) so it leaves the hangar and travels with the job.
    for (const auto& ct : cargo) {
        DBQueryResult ires;
        std::vector<uint32> itemIDs;
        if (sDatabase.RunQuery(ires,
            "SELECT itemID FROM entity WHERE ownerID = %u AND locationID = %u AND flag = %u AND typeID = %u AND quantity > 0 AND singleton = 0 LIMIT 5",
            charID, stationID, (uint32)flagHangar, ct.typeID))
        {
            DBResultRow irow;
            while (ires.GetRow(irow))
                itemIDs.push_back(irow.GetUInt(0));
        }
        uint32 remaining = ct.qty;
        for (uint32 itemID : itemIDs) {
            if (remaining == 0)
                break;
            InventoryItemRef itm = sItemFactory.GetItemRef(itemID);
            if (itm.get() == nullptr)
                continue;
            uint32 take = itm->quantity();
            if (take > remaining) take = remaining;
            InventoryItemRef part = itm;
            if (itm->quantity() > take)
                part = itm->Split(take);
            if (part.get() == nullptr)
                continue;
            part->ChangeOwner(1, false);   // locked into the contract
            part->SaveItem();
            // ctrItems row.
            DBerror ierr;
            sDatabase.RunQuery(ierr,
                "INSERT INTO ctrItems (contractId, itemID, quantity, itemTypeID, inCrate, parentID,"
                "  productivityLevel, materialLevel, isCopy, licensedProductionRunsRemaining, damage, flagID)"
                " VALUES (%u, %u, %u, %u, 0, 0, 0, 0, 0, 0, 0, 0)",
                contractId, part->itemID(), take, ct.typeID);
            remaining -= take;
        }
    }

    _log(BOT__MESSAGE, "BotMgr: %u packed %u x types (%.0f m3, reward %.0f ISK) into courier contract %u -> station %u.",
         charID, (uint32)cargo.size(), totalVol, (double)reward, contractId, endStation);
    return contractId;
}

// A bot docked at the trade hub (Jita) sells the real stock in its hangar into
// the best resting buy orders for each type — closing the ISK loop: ore/faction
// loot that miners/ratters hauled to the hub actually becomes ISK in the bot's
// wallet (offline transfer), not just goods gathering dust in a hangar.
double BotMgr::SellStockAtHub(uint32 sysID, uint32 stationID, uint32 charID)
{
    if (sysID == 0 || stationID == 0 || charID == 0)
        return 0.0;
    if (!IsTradeHub(sysID))
        return 0.0;   // only sell at the market hub

    // All real stock this bot owns in this station's hangar (non-ship stacks).
    struct StockType { uint16 typeID; uint32 qty; };
    std::vector<StockType> stock;
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT typeID, SUM(quantity) FROM entity"
        " WHERE ownerID = %u AND locationID = %u AND flag = %u AND quantity > 0 AND singleton = 0"
        " GROUP BY typeID", charID, stationID, (uint32)flagHangar))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            StockType st; st.typeID = (uint16)row.GetUInt(0); st.qty = row.GetUInt(1);
            stock.push_back(st);
        }
    }
    if (stock.empty())
        return 0.0;

    double totalISK = 0.0;
    for (const auto& st : stock) {
        // Find the best (highest-priced) resting buy order at this station for
        // this type that still has room, not owned by the seller.
        DBQueryResult bres;
        uint32 bestOrder = 0;
        if (sDatabase.RunQuery(bres,
            "SELECT orderID FROM mktOrders"
            " WHERE stationID = %u AND typeID = %u AND bid = 1 AND volRemaining > 0 AND ownerID <> %u"
            " ORDER BY price DESC LIMIT 1", stationID, st.typeID, charID))
        {
            DBResultRow brow;
            if (bres.GetRow(brow))
                bestOrder = brow.GetUInt(0);
        }
        if (bestOrder == 0)
            continue;   // nobody is buying this here — leave it in the hangar

        // Sell into it with the bot's real stack. Grab the stack item.
        DBQueryResult ires;
        uint32 stackItemID = 0;
        if (sDatabase.RunQuery(ires,
            "SELECT itemID FROM entity"
            " WHERE ownerID = %u AND locationID = %u AND flag = %u AND typeID = %u AND quantity > 0 AND singleton = 0"
            " LIMIT 1", charID, stationID, (uint32)flagHangar, st.typeID))
        {
            DBResultRow irow;
            if (ires.GetRow(irow))
                stackItemID = irow.GetUInt(0);
        }
        if (stackItemID == 0)
            continue;
        InventoryItemRef iRef = sItemFactory.GetItemRef(stackItemID);
        if (iRef.get() == nullptr)
            continue;

        double got = sMktMgr.SellStockIntoBuyOrder(charID, bestOrder, iRef, st.qty, stationID, st.typeID);
        totalISK += got;
    }

    if (totalISK > 0)
        _log(BOT__MESSAGE, "BotMgr: %u sold hub stock at station %u, +%.0f ISK.",
             charID, stationID, totalISK);
    return totalISK;
}


// Find a chelobot by charID in a system.
static PlayerBot* BotMgr_FindInSystem(SystemManager* sm, uint32 charID)
{
    if (sm == nullptr) return nullptr;
    for (auto& [id, se] : sm->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr) continue;
        PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (pb != nullptr && pb->GetBotCharID() == charID) return pb;
    }
    return nullptr;
}

// Flag a courier to dock at its destination station (delivery happens on dock).
static void BotMgr_RequestCourierDock(PlayerBot* pb)
{
    if (pb == nullptr) return;
    pb->ClearTravel();
    pb->RequestDock();
}

// Per-tick fallback: if a haul arrived at its destination but the bot never got
// to dock (e.g. the system has no players so docking never runs), complete it.
void BotMgr::ProcessHaulDeliveries()
{
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    time_t now = time(nullptr);
    for (auto it = m_hauls.begin(); it != m_hauls.end(); ) {
        CourierHaul& haul = it->second;
        bool done = false;
        if (haul.arrivedAt != 0 && (now - haul.arrivedAt) > 45) {
            CompleteContract(it->first, haul.endSys);
            done = true;
        } else if (haul.arrivedAt != 0 && haul.route.empty()) {
            // arrived + no more hops: the dock path is expected; if the courier
            // has already left the docked list it's on its way — leave pending.
        }
        if (done) it = m_hauls.erase(it);
        else ++it;
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
                if (prof == PlayerBot::BotProfession::Industrialist) {
                    // Producer visits either its POS or its colony planet so the
                    // activity is VISIBLE (then it lingers and docks again later).
                    bool goToColony = (MakeRandomInt(0, 1) == 1);
                    GPoint dest;
                    bool haveDest = false;

                    if (goToColony) {
                        DBQueryResult cr;
                        uint32 planetID = 0;
                        if (sDatabase.RunQuery(cr,
                            "SELECT planetID FROM botColonies WHERE charID = %u", npb->GetBotCharID())) {
                            DBResultRow r; if (cr.GetRow(r)) planetID = r.GetUInt(0);
                        }
                        if (planetID != 0) {
                            DBQueryResult pr;
                            if (sDatabase.RunQuery(pr,
                                "SELECT x, y, z, IFNULL(radius,0) FROM mapDenormalize WHERE itemID = %u", planetID)) {
                                DBResultRow r;
                                if (pr.GetRow(r)) {
                                    dest.x = r.GetDouble(0) + r.GetDouble(3) + 200000.0;
                                    dest.y = r.GetDouble(1);
                                    dest.z = r.GetDouble(2);
                                    haveDest = true;
                                }
                            }
                        }
                    }

                    if (!haveDest) {
                        SystemEntity* tower = nullptr;
                        for (auto& [eid, e] : pSystem->GetEntities()) {
                            if (e != nullptr && e->GetTowerSE() != nullptr) { tower = e; break; }
                        }
                        if (tower != nullptr) {
                            dest = tower->GetPosition();
                            dest.x += 9000.0;
                            haveDest = true;
                        }
                    }

                    if (haveDest)
                        npb->DestinyMgr()->WarpTo(dest, 0);
                } else if (prof == PlayerBot::BotProfession::Courier || prof == PlayerBot::BotProfession::Trader) {
                    npb->MarkForTravel();   // visible warp to the gate, then cross
                }
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
            // Which station is this bot docking at? (first station SE in the
            // system — the same one it approached above). Used later so a
            // docked trader works the correct station's order book.
            uint32 dockStationID = 0;
            for (auto& [sid, sse] : pSystem->GetStaticEntities()) {
                if (sse != nullptr && sse->GetStationSE() != nullptr) { dockStationID = sse->GetID(); break; }
            }
            DockedBot db;
                db.charID = pb->GetBotCharID();
                db.name = pb->GetBotName();
                db.corpID = pb->GetBotCorpID();
                db.allianceID = pb->GetBotAllianceID();
                db.profession = (uint8)pb->GetProfession();
                db.stationID = dockStationID;
                // Traders/market guys sit longer; miners refine/sell quickly then head out.
                db.undockAt = now + (pb->GetProfession() == PlayerBot::BotProfession::Trader
                                     ? MakeRandomInt(300, 1800)    // 5-30 min at market
                                     : MakeRandomInt(30, 300));    // 0.5-5 min for everyone else
            m_docked[pSystem->GetID()].push_back(db);
            _log(BOT__MESSAGE, "BotMgr: %s(%u) docking at station %u in system %u.",
                 db.name.c_str(), db.charID, dockStationID, pSystem->GetID());
            pb->ClearDockRequest();
            pb->RecallDrones();   // scoop drones before docking
            // Courier haul arrived at its destination and now docked → deliver the
            // contract (real physical "dock to complete") and end the run.
            auto haulIt = m_hauls.find(db.charID);
            if (haulIt != m_hauls.end() && haulIt->second.arrivedAt != 0
                && haulIt->second.endSys == pSystem->GetID()) {
                CompleteContract(db.charID, haulIt->second.endSys);
                _log(BOT__MESSAGE, "BotMgr: courier %s(%u) docked at destination system %u — haul complete.",
                     db.name.c_str(), db.charID, pSystem->GetID());
                m_hauls.erase(haulIt);
                // Let the courier sit for a bit before heading out again.
                db.undockAt = now + MakeRandomInt(120, 600);
            }
            // Missioner docked to report in a run (it carried salvage back) — pay
            // the agent mission reward into its wallet (checked before the cargo
            // deposit below empties the hold).
            if (pb->GetProfession() == PlayerBot::BotProfession::Missioner && pb->HasCargo())
                PayMissionReward(pb);
            // Stage-2 physical goods: a miner/ratter/hacker deposits its real
            // cargo hold into the station hangar when it docks, so the station
            // accumulates physical minerals/loot a trader can later pack into a
            // courier contract (which a courier hauls to another station).
            if (dockStationID != 0 && pb->HasCargo()) {
                double dep = pb->DepositCargoAtStation(dockStationID);
                if (dep > 0)
                    _log(BOT__MESSAGE, "BotMgr: %s(%u) deposited %.0f units of cargo at station %u.",
                         db.name.c_str(), db.charID, dep, dockStationID);
            }
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

        // Build a line from THIS bot's real situation: its profession and what is
        // actually happening around it (fighting, gate, empty system). A miner
        // talks about ore/belts, a hunter about ganks, a courier about hauls —
        // never a generic "want to PvP?" from a bot sitting in a belt.
        std::string msg = BuildBotSmalltalkLine(a, b, pSystem);

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

// Compose a believable local line for a chelobot: profession-flavoured and
// grounded in what is ACTUALLY happening to it right now (fighting, sitting at a
// gate, an empty system), addressed at another bot in the same system. Never a
// generic off-topic phrase (a miner doesn't shout "gf pvp" from a belt).
std::string BotMgr::BuildBotSmalltalkLine(PlayerBot* a, PlayerBot* b, SystemManager* pSystem)
{
    if (a == nullptr)
        return "";
    std::string sysName = pSystem != nullptr ? pSystem->GetName() : "this system";

    bool inFight = a->GetAIMgr() != nullptr && a->GetAIMgr()->IsFighting();
    bool aggro = a->IsAggressed();
    // Near a gate? (hunter bait / gate camp / waiting to jump)
    bool nearGate = a->IsNearGate(150000.0);

    // Profession-flavoured line pools.
    static const char* miner[] = {
        "belt's been treating me ok, %s.",
        "my strip miners are chewing through %s ore like mad.",
        "anyone got a spare Mining Laser V book? haul keeps getting full.",
        "think I'll refine this load and call it a day.",
    };
    static const char* ratHunter[] = {
        "anoms are quiet in %s today.",
        "just popped a nice rat spawn, wallet's happy.",
        "this system's rats hit harder than the last one.",
        "anyone seen a good haven around %s?",
    };
    static const char* hacker[] = {
        "datacore prices better hold, I've got a hold full.",
        "relic site in %s was worth the scan.",
        "these data sites are getting camped lately.",
        "found a nice relic, decryptors are mine now.",
    };
    static const char* explorer[] = {
        "found a sig in %s, scanning it down.",
        "wormhole in here earlier, anyone peeked?",
        "probes out, system's got a few signatures.",
        "that null static is tempting.",
    };
    static const char* trader[] = {
        "buy orders up in %s, spread's nice.",
        "market's moving, good day to be a trader.",
        "someone undercut me again, classic.",
        "haul of goods just went out, profit's in.",
    };
    static const char* courier[] = {
        "courier job open to the hub if anyone's hauling.",
        "just moved a load through %s, easy isk.",
        "anyone need something moved to Jita?",
        "cargo's in, contract's up, rates are fair.",
    };
    static const char* hunter[] = {
        "anyone in %s worth engaging?",
        "scan shows a target, thinking about it.",
        "this gate's quiet, might camp a bit.",
        "gf if anyone's up for a scrap.",
    };

    const char** pool = miner; int n = sizeof(miner)/sizeof(miner[0]);
    switch (a->GetProfession()) {
        case PlayerBot::BotProfession::RatHunter: pool = ratHunter; n = sizeof(ratHunter)/sizeof(ratHunter[0]); break;
        case PlayerBot::BotProfession::Hacker:    pool = hacker;    n = sizeof(hacker)/sizeof(hacker[0]);    break;
        case PlayerBot::BotProfession::Explorer:  pool = explorer;  n = sizeof(explorer)/sizeof(explorer[0]); break;
        case PlayerBot::BotProfession::Trader:    pool = trader;    n = sizeof(trader)/sizeof(trader[0]);    break;
        case PlayerBot::BotProfession::Courier:   pool = courier;   n = sizeof(courier)/sizeof(courier[0]);  break;
        case PlayerBot::BotProfession::Hunter:    pool = hunter;    n = sizeof(hunter)/sizeof(hunter[0]);    break;
        case PlayerBot::BotProfession::Miner:
        default:                                  break;
    }

    // When the bot is under attack or mid-fight, override with a combat line —
    // that's the most salient thing happening to it right now.
    std::string msg;
    if (inFight || aggro) {
        static const char* fight[] = {
            "bit busy here, someone's on me.",
            "ffs, getting shot at in %s.",
            "who's engaging me??",
            "tackled, need backup maybe.",
        };
        msg = fight[MakeRandomInt(0, 3)];
        std::string s = msg;
        size_t pos = s.find("%s");
        if (pos != std::string::npos)
            s.replace(pos, 2, sysName);
        return s;
    }

    // Otherwise a profession line, substituting the system name if present.
    msg = pool[MakeRandomInt(0, n - 1)];
    {
        std::string s = msg;
        size_t pos = s.find("%s");
        if (pos != std::string::npos)
            s.replace(pos, 2, sysName);
        msg = s;
    }

    // Sometimes mention the other bot by name.
    if (b != nullptr && MakeRandomInt(0, 99) < 35)
        msg = std::string(b->GetBotName()) + ", " + msg;
    return msg;
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
    // When contracts pile up — a big public backlog (>20) or one very stale
    // (>1 day) — bots accept them even from other loaded systems so the market
    // keeps moving instead of leaving goods parked at a station forever.
    if (!m_initalized || !sConfig.playerBots.Enabled)
        return;
    if (sEntityList.GetSystems().empty())
        return;

    // Global backlog of public, unaccepted courier contracts.
    uint32 backlog = 0;
    {
        DBQueryResult cnt;
        if (sDatabase.RunQuery(cnt,
            "SELECT COUNT(*) FROM ctrContracts"
            " WHERE contractType = 3 AND status = 0 AND acceptorID = 0 AND isPrivate = 0"))
        {
            DBResultRow cr;
            if (cnt.GetRow(cr)) backlog = cr.GetUInt(0);
        }
    }

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT contractId, startStationID, startSolarSystemID, endSolarSystemID, reward, dateIssued, volume"
        " FROM ctrContracts"
        " WHERE contractType = 3"        // courier
        "   AND status = 0"              // created, not yet accepted
        "   AND acceptorID = 0"          // nobody picked it up
        "   AND isPrivate = 0"           // public contract
        " ORDER BY dateIssued ASC LIMIT 15"))
    {
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 contractID = row.GetUInt(0);
        uint32 startSys = row.GetUInt(2);
        uint32 endSys = row.GetUInt(3);
        int64 reward = row.GetInt64(4);
        int64 dateIssued = row.GetInt64(5);
        double volume = row.GetDouble(6);

        // Age of the contract (FILETIME, 100ns ticks). Skip fresh ones at normal
        // cadence so a real player can still grab a new job — but when the market
        // is backed up (backlog > 20) or a job is very stale (>1 day), bots take
        // even fresh/any contracts to clear the queue.
        int64 age = (dateIssued > 0) ? (GetFileTimeNow() - dateIssued) : 0;
        bool urgent = (backlog > 20) || (age > EvE::Time::Day);
        if (!urgent && age < 5LL * EvE::Time::Minute)
            continue;

        // Prefer a free courier in the contract's start system...
        PlayerBot* courier = FindFreeCourier(startSys);
        // ...but for urgent/backed-up contracts accept from ANY loaded system.
        if (courier == nullptr && urgent)
            courier = FindFreeCourier(0);

        if (courier == nullptr)
            continue;   // no free courier right now — leave contract for later

        // Accept the contract: mark acceptorID and status.
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE ctrContracts SET acceptorID = %u, status = 1, dateAccepted = %lli WHERE contractId = %u",
            courier->GetBotCharID(), (int64)GetFileTimeNow(), contractID);

        _log(BOT__MESSAGE, "BotMgr: courier %s(%u) accepted contract %u (reward %.0f ISK, %.0f m3) to system %u%s.",
             courier->GetBotName().c_str(), courier->GetBotCharID(), contractID, (double)reward, volume, endSys,
             (urgent ? " [urgent]" : ""));

        if (endSys != 0) {
            // Big cargo (>10,000 m3) goes by JUMP FREIGHTER through a cyno —
            // lights a visible cyno, holds an interception window (players can
            // warp in and shoot it or its guards), then jumps. Guards protect it.
            if (volume > 10000) {
                courier->StartJumpFreighter(endSys);
            } else {
                // Small cargo: fly through gates normally, gate by gate (a real
                // haul — visible warps/jumps through each system). Compute a BFS
                // route from the courier's current system to the destination.
                uint32 curSys = (courier->SystemMgr() != nullptr) ? courier->SystemMgr()->GetID() : startSys;
                std::vector<uint32> route;
                if (ComputeHaulRoute(curSys, endSys, route) && route.size() > 1) {
                    CourierHaul haul;
                    haul.contractID = contractID;
                    haul.endSys = endSys;
                    haul.endStation = 0;
                    haul.route.assign(route.begin() + 1, route.end());   // skip current system
                    haul.arrivedAt = 0;
                    m_hauls[courier->GetBotCharID()] = std::move(haul);
                    courier->SetTravelDestination(route[1]);
                    courier->MarkForTravel(route[1]);
                    _log(BOT__MESSAGE, "BotMgr: courier %s(%u) hauling contract %u via gate route (%zu jumps) to system %u.",
                         courier->GetBotName().c_str(), courier->GetBotCharID(), contractID,
                         route.size() - 1, endSys);
                } else {
                    // No path or already there — fall back to the direct hop.
                    courier->SetTravelDestination(endSys);
                    courier->MarkForTravel(endSys);
                }
            }
            // Reward ISK is paid on successful delivery (handled when the
            // freighter/courier completes the run), not at acceptance.
        }
    }
}

PlayerBot* BotMgr::FindFreeCourier(uint32 systemID)
{
    if (systemID != 0) {
        // only that system — and only if it is actually loaded
        if (!sEntityList.IsSystemLoaded(systemID))
            return nullptr;
        SystemManager* sm = sEntityList.FindOrBootSystem(systemID);
        if (sm == nullptr)
            return nullptr;
        for (auto& [id, se] : sm->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr && pb->GetProfession() == PlayerBot::BotProfession::Courier
                && !pb->WantsToTravel() && !pb->IsTraveling() && !pb->WantsDock())
                return pb;
        }
        return nullptr;
    }
    // scan every loaded system for a free courier
    for (auto& [sysID, sm] : sEntityList.GetSystems()) {
        if (sm == nullptr)
            continue;
        for (auto& [id, se] : sm->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* pb = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (pb != nullptr && pb->GetProfession() == PlayerBot::BotProfession::Courier
                && !pb->WantsToTravel() && !pb->IsTraveling() && !pb->WantsDock())
                return pb;
        }
    }
    return nullptr;
}

// A courier bot that has reached the destination system completes the courier
// contracts it accepted: the cargo (ctrItems) is moved to the issuer's hangar
// at the end station and the reward is paid from the issuer to the courier.
// Mirrors what ContractProxy::CompleteContract does for a player courier, but
// runs client-less (the courier is an offline character, so ISK uses the
// offline wallet path and items are moved straight in the DB).
void BotMgr::CompleteContract(uint32 charID, uint32 destSystem)
{
    if (charID == 0 || destSystem == 0)
        return;

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT contractId, endStationID, reward"
        " FROM ctrContracts"
        " WHERE contractType = 3"        // courier
        "   AND status = 1"              // accepted, in progress
        "   AND acceptorID = %u"         // this courier is hauling it
        "   AND endSolarSystemID = %u"   // and it just reached the destination system
        "   AND dateCompleted = 0",
        charID, destSystem))
    {
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 contractID  = row.GetUInt(0);
        uint32 endStation   = row.GetUInt(1);
        int64  reward       = row.GetInt64(2);

        // Who issued the contract — their hangar at the end station receives
        // the delivered goods (use the corp's office hangar for corp contracts).
        DBQueryResult ires;
        uint32 issuerID = 0;
        if (sDatabase.RunQuery(ires,
            "SELECT issuerID, issuerCorpID, forCorp FROM ctrContracts WHERE contractId = %u", contractID))
        {
            DBResultRow irow;
            if (ires.GetRow(irow)) {
                issuerID = irow.GetUInt(0);
                bool forCorp = irow.GetBool(2);
                if (forCorp && irow.GetUInt(1) != 0)
                    issuerID = irow.GetUInt(1);
            }
        }
        if (issuerID == 0)
            continue;

        // Deliver the cargo: any physical items locked in the contract move to
        // the issuer's hangar at the end station. (Bot courier contracts have no
        // ctrItems — real volume only exists for player courier contracts.)
        std::vector<uint32> cargo;
        DBQueryResult itres;
        if (sDatabase.RunQuery(itres, "SELECT itemID FROM ctrItems WHERE contractId = %u AND itemID != 0", contractID)) {
            DBResultRow itrow;
            while (itres.GetRow(itrow))
                cargo.push_back(itrow.GetUInt(0));
        }
        EVEItemFlags destFlag = flagHangar;
        for (uint32 itemID : cargo) {
            InventoryItemRef itm = sItemFactory.GetItemRef(itemID);
            if (itm.get() == nullptr)
                continue;
            // Contract items were parked on the issuer (owner 1 / limbo) while
            // the courier hauled them; hand them to the issuer at the destination.
            itm->ChangeOwner(issuerID, true);
            itm->Move(endStation, destFlag, true);
        }

        // Pay the courier the reward from the issuer's wallet.
        if (reward > 0) {
            std::string reason = "Reward for courier contract";
            AccountService::TransferFunds(issuerID, charID, (double)reward, reason.c_str(),
                Journal::EntryType::ContractReward, contractID,
                Account::KeyType::Cash, Account::KeyType::Cash);
        }

        DBerror err;
        if (!sDatabase.RunQuery(err,
            "UPDATE ctrContracts SET status = 4, dateCompleted = %lli WHERE contractId = %u",
            (int64)GetFileTimeNow(), contractID))
        {
            codelog(DATABASE__ERROR, "CompleteContract() failed to finish %u: %s", contractID, err.c_str());
            continue;
        }

        _log(BOT__MESSAGE, "BotMgr: courier %u delivered contract %u (%zu items) to station %u, +%.0f ISK reward.",
             charID, contractID, cargo.size(), endStation, (double)reward);
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
        "Russian chat is absolutely allowed and natural here (EVE has a huge RU community: ratted 'не в чате', "
        "'ц', 'оф', 'ку', 'оф', сленг: fit/фит, гейт, лоу, нули, ратта). "
        "You are a real living human player — write the way a real person types in a game chat: casual, "
        "imperfect, with typos and dropped words, mixed registers, your own habits. Don't chase perfect grammar "
        "or full sentences. Use varied speech patterns, idioms and local flavor. "
        "You may be blunt, rude or dismissive if the situation calls for it — a grumpy veteran, a cocky PvP'er, "
        "a sarcastic miner — but stay within EVE's rules: no real-life hate speech, threats, slurs or anything "
        "that would get a real account banned. Being human and rough is fine; being toxic is not.";

    // Situational context the bot IS aware of (it's in the same system/bubble):
    // this keeps replies grounded and prevents the classic "I'm alone ratting"
    // tell when the player is standing right next to the bot.
    {
        bool playerNear = false;
        bool inCombat = responder->GetAIMgr() != nullptr && responder->GetAIMgr()->IsFighting();
        SystemBubble* bub = responder->SysBubble();
        if (bub != nullptr) {
            std::vector<Client*> clients;
            bub->GetPlayers(clients);
            playerNear = !clients.empty();
        }
        // Where the bot actually is right now (user rule): system + security —
        // cached (one lookup per system per session, game-thread only).
        {
            static std::map<int32, std::pair<std::string, float>> sSysCache;
            auto cit = sSysCache.find(channelID);
            if (cit == sSysCache.end()) {
                DBQueryResult sres;
                std::pair<std::string, float> v;
                if (sDatabase.RunQuery(sres,
                    "SELECT solarSystemName, COALESCE(security,1) FROM mapSolarSystems"
                    " WHERE solarSystemID = %u", (uint32)channelID))
                {
                    DBResultRow srow;
                    if (sres.GetRow(srow))
                        v = { srow.GetText(0) ? srow.GetText(0) : "", (float)srow.GetFloat(1) };
                }
                sSysCache[channelID] = v;
                cit = sSysCache.find(channelID);
            }
            const std::string& sysName = cit->second.first;
            float sysSec = cit->second.second;
            if (!sysName.empty()) {
                char sb[32];
                snprintf(sb, sizeof(sb), "%.1f", sysSec);
                systemHint += " You are currently in the " + sysName + " system (security " + sb + ").";
                // space-event awareness: keep the mind on TODAY's surroundings
                if (sysSec < 0.5f)
                    systemHint += " This is dangerous space — expected behaviour here: gates plates, cloak, d-scan.";
            }
        }
        if (playerNear) {
            systemHint += " A pilot is in the same grid as you and can see you — "
                          "do NOT claim you are alone somewhere (no 'I'm all alone in an anomaly' "
                          "when someone is literally next to you). Reference the other pilot's "
                          "presence naturally if it fits.";
        }
        if (inCombat) {
            systemHint += " You are currently in a fight — mention it if it fits "
                          "('bit busy', 'in a scrap', etc.) but don't make it the whole reply.";
        }
        // Line the bot is CURRENTLY busy with (its profession activity phrase
        // is built by the same code as smalltalk) — grounding the reply in what
        // the bot is actually doing RIGHT NOW, per its job.
        std::string doing = BuildBotSmalltalkLine(responder, nullptr, pSystem);
        if (!doing.empty())
            systemHint += " Right now you are: " + doing + ".";
    }

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
