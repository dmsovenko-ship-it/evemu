#include "eve-server.h"
#include "npc/BotMemory.h"

/*
 * @file BotMemory.cpp
 * Persistent per-bot learning stats. Loaded once per bot spawn, saved on change
 * (throttled) so experience survives restarts.
 */

BotMemory::BotMemory(uint32 charID)
: m_charID(charID),
  wins(0), losses(0), kills(0), deaths(0),
  chatLines(0), chatReplies(0),
  ratKills(0), mineRuns(0), tradeRuns(0), hackRuns(0),
  pvpMistakes(0),
  profession(0xFF),
  skillLevel(0xFF),
  tradeProfit(0), tradeLosses(0),
  m_dirty(false)
{
}

void BotMemory::Load()
{
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT wins, losses, kills, deaths, chatLines, chatReplies,"
        "       ratKills, mineRuns, tradeRuns, hackRuns, pvpMistakes, profession,"
        "       skillLevel, tradeProfit, tradeLosses"
        " FROM botMemory WHERE charID = %u", m_charID))
    {
        DBResultRow row;
        if (res.GetRow(row)) {
            wins = row.GetUInt(0);
            losses = row.GetUInt(1);
            kills = row.GetUInt(2);
            deaths = row.GetUInt(3);
            chatLines = row.GetUInt(4);
            chatReplies = row.GetUInt(5);
            ratKills = row.GetUInt(6);
            mineRuns = row.GetUInt(7);
            tradeRuns = row.GetUInt(8);
            hackRuns = row.GetUInt(9);
            if (row.ColumnCount() > 10) {
                pvpMistakes = row.GetUInt(10);
                profession = (uint8)row.GetUInt(11);
            }
            if (row.ColumnCount() > 12) {
                skillLevel = (uint8)row.GetUInt(12);
                tradeProfit = row.GetInt64(13);
                tradeLosses = row.GetUInt(14);
            }
        }
    }
    m_dirty = false;
}

void BotMemory::Save() const
{
    if (!m_dirty)
        return;
    DBerror err;
    if (!sDatabase.RunQuery(err,
        "INSERT INTO botMemory (charID, wins, losses, kills, deaths, chatLines, chatReplies,"
        "                       ratKills, mineRuns, tradeRuns, hackRuns, pvpMistakes, profession,"
        "                       skillLevel, tradeProfit, tradeLosses, lastUpdate)"
        " VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %lli, %u, NOW())"
        " ON DUPLICATE KEY UPDATE"
        "  wins = VALUES(wins), losses = VALUES(losses), kills = VALUES(kills),"
        "  deaths = VALUES(deaths), chatLines = VALUES(chatLines), chatReplies = VALUES(chatReplies),"
        "  ratKills = VALUES(ratKills), mineRuns = VALUES(mineRuns), tradeRuns = VALUES(tradeRuns),"
        "  hackRuns = VALUES(hackRuns), pvpMistakes = VALUES(pvpMistakes), profession = VALUES(profession),"
        "  skillLevel = VALUES(skillLevel), tradeProfit = VALUES(tradeProfit), tradeLosses = VALUES(tradeLosses),"
        "  lastUpdate = NOW()",
        m_charID, wins, losses, kills, deaths, chatLines, chatReplies,
        ratKills, mineRuns, tradeRuns, hackRuns, pvpMistakes, profession,
        skillLevel, (int64)tradeProfit, tradeLosses))
    {
        codelog(DATABASE__ERROR, "BotMemory::Save() failed for %u: %s", m_charID, err.c_str());
    }
}

uint32 BotMemory::PracticeForNextLevel(uint8 cur)
{
    // Practice thresholds to climb from one tier to the next. Roughly doubles
    // each tier so early levels come fast and late ones take real time:
    //   0->1: 4   actions, 1->2: 10, 2->3: 22, 3->4: 46, 4->5: 95.
    static const uint32 need[] = { 4, 10, 22, 46, 95 };
    if (cur >= 5)
        return 0;   // already maxed
    return need[cur];
}

float BotMemory::GetAggression() const
{
    // Win/loss differential normalized to [-1..+1]. No fights yet → neutral 0.
    uint32 total = wins + losses;
    if (total == 0)
        return 0.0f;
    float diff = (float)((int)wins - (int)losses);
    return diff / (float)total;   // +1 all wins, -1 all losses
}

float BotMemory::GetChatQuality() const
{
    if (chatLines == 0)
        return 0.0f;
    float ratio = (float)chatReplies / (float)chatLines;
    return ratio > 1.0f ? 1.0f : ratio;
}

float BotMemory::GetActivitySkill() const
{
    // Practice makes perfect: total profession actions, saturating at ~30.
    uint32 total = ratKills + mineRuns + tradeRuns + hackRuns;
    float skill = (float)total / 30.0f;
    return skill > 1.0f ? 1.0f : skill;
}

float BotMemory::GetPvpSkill() const
{
    // Judgement of a fight improves with real PvP reps (wins+kills) and is hurt
    // by misjudged ones. Saturates at 1.0 (a veteran who rarely misreads a fight).
    uint32 exp = wins + kills + deaths + losses;
    float base = (float)exp / 8.0f;   // ~8 fights to reach "average" judgement
    if (base > 1.0f) base = 1.0f;
    float penalty = pvpMistakes > 0 ? (float)pvpMistakes * 0.25f : 0.0f;
    float skill = base - penalty;
    if (skill < 0.0f) skill = 0.0f;
    return skill;
}

float BotMemory::GetTradeConfidence() const
{
    // Market self-learning signal, normalised to [-1..+1]. Trades only recently
    // started mattering, so scale the ISK result softly (~20M ISK = full swing).
    if (tradeProfit == 0 && tradeLosses == 0)
        return 0.0f;
    float isk = (float)tradeProfit / 20000000.0f;   // 20M ISK net = ±1.0
    if (isk > 1.0f) isk = 1.0f;
    if (isk < -1.0f) isk = -1.0f;
    // Losing fills push confidence down harder than the raw ISK alone (each loss
    // is a mistake the bot should learn from, not just a small negative tick).
    float lossPenalty = (float)tradeLosses * 0.15f;
    isk -= lossPenalty;
    if (isk < -1.0f) isk = -1.0f;
    return isk;
}
