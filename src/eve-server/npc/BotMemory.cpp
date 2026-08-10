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
  m_dirty(false)
{
}

void BotMemory::Load()
{
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        "SELECT wins, losses, kills, deaths, chatLines, chatReplies"
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
        "INSERT INTO botMemory (charID, wins, losses, kills, deaths, chatLines, chatReplies, lastUpdate)"
        " VALUES (%u, %u, %u, %u, %u, %u, %u, NOW())"
        " ON DUPLICATE KEY UPDATE"
        "  wins = VALUES(wins), losses = VALUES(losses), kills = VALUES(kills),"
        "  deaths = VALUES(deaths), chatLines = VALUES(chatLines), chatReplies = VALUES(chatReplies),"
        "  lastUpdate = NOW()",
        m_charID, wins, losses, kills, deaths, chatLines, chatReplies))
    {
        codelog(DATABASE__ERROR, "BotMemory::Save() failed for %u: %s", m_charID, err.c_str());
    }
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
