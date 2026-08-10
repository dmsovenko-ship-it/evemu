#ifndef EVEMU_PLAYERBOT_BOTMEMORY_H_
#define EVEMU_PLAYERBOT_BOTMEMORY_H_

#include "eve-compat.h"
#include "eve-common.h"

/**
 * @brief Persistent self-learning memory for a simulated player.
 *
 * Tracks combat outcomes (wins/losses/kills/deaths) and chat behaviour
 * (lines sent vs replies received) per bot in the `botMemory` table, so each
 * bot learns from experience:
 *   - aggression: a bot that keeps winning grows bolder; a bot that keeps dying
 *     becomes cautious (flees more often, engages only when clearly favoured).
 *   - chat: lines that drew a reply are "good" — the bot reuses that tone/slang.
 *
 * @author bot-infrastructure
 */
class BotMemory
{
public:
    BotMemory(uint32 charID);
    ~BotMemory() = default;

    void Load();                 // read from DB (or init defaults)
    void Save() const;           // write back to DB

    /* combat learning */
    void RecordWin()   { ++wins;   RecordChange(); }
    void RecordLoss()  { ++losses; RecordChange(); }
    void RecordKill()  { ++kills;  RecordChange(); }
    void RecordDeath() { ++deaths; RecordChange(); }

    /* chat learning */
    void RecordChatLine()    { ++chatLines;   RecordChange(); }
    void RecordChatReply()   { ++chatReplies; RecordChange(); }

    /* derived behaviour modifiers, in [-1..+1] */
    // +1 = very bold (wins far exceed losses); -1 = very cautious.
    float GetAggression() const;
    // 0..1: how "well received" this bot's chat lines are (reply ratio).
    float GetChatQuality() const;

    uint32 GetCharID() const    { return m_charID; }
    uint32 GetWins() const      { return wins; }
    uint32 GetLosses() const    { return losses; }
    uint32 GetKills() const     { return kills; }
    uint32 GetDeaths() const    { return deaths; }

private:
    void RecordChange() { m_dirty = true; }

    uint32 m_charID;
    uint32 wins, losses, kills, deaths;
    uint32 chatLines, chatReplies;
    bool m_dirty;
};

#endif  // EVEMU_PLAYERBOT_BOTMEMORY_H_
