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

    /* persistent profession (survives respawns so the bot keeps its job) */
    void SetProfession(uint8 p)  { profession = p; RecordChange(); }
    uint8 GetProfession() const  { return profession; }
    bool HasProfession() const   { return profession != 0xFF; }

    /* combat learning */
    void RecordWin()   { ++wins;   RecordChange(); }
    void RecordLoss()  { ++losses; RecordChange(); }
    void RecordKill()  { ++kills;  RecordChange(); }
    void RecordDeath() { ++deaths; RecordChange(); }
    // A PvP engagement the bot read wrong (attacked and lost, or fled from a
    // winnable fight). Marks inexperience; the bot learns to judge fights better.
    void RecordPvpMistake() { ++pvpMistakes; RecordChange(); }

    /* chat learning */
    void RecordChatLine()    { ++chatLines;   RecordChange(); }
    void RecordChatReply()   { ++chatReplies; RecordChange(); }

    /* profession learning */
    void RecordRatKill()     { ++ratKills;  RecordChange(); }
    void RecordMineRun()     { ++mineRuns;  RecordChange(); }
    void RecordTradeRun()    { ++tradeRuns; RecordChange(); }
    void RecordHackRun()     { ++hackRuns;  RecordChange(); }

    /* derived behaviour modifiers, in [-1..+1] */
    // +1 = very bold (wins far exceed losses); -1 = very cautious.
    float GetAggression() const;
    // 0..1: how "well received" this bot's chat lines are (reply ratio).
    float GetChatQuality() const;
    // 0..1: how practiced this bot is at its job (activity count, saturating).
    float GetActivitySkill() const;
    // 0..1: how good the bot is at judging a fight. Grows with PvP experience
    // (wins+kills) and shrinks the more it misjudges. Novices make mistakes.
    float GetPvpSkill() const;

    uint32 GetCharID() const    { return m_charID; }
    uint32 GetWins() const      { return wins; }
    uint32 GetLosses() const    { return losses; }
    uint32 GetKills() const     { return kills; }
    uint32 GetDeaths() const    { return deaths; }
    uint32 GetRatKills() const  { return ratKills; }
    uint32 GetMineRuns() const  { return mineRuns; }
    uint32 GetTradeRuns() const { return tradeRuns; }
    uint32 GetHackRuns() const  { return hackRuns; }
    uint32 GetPvpMistakes() const { return pvpMistakes; }

private:
    void RecordChange() { m_dirty = true; }

    uint32 m_charID;
    uint32 wins, losses, kills, deaths;
    uint32 chatLines, chatReplies;
    uint32 ratKills, mineRuns, tradeRuns, hackRuns;
    uint32 pvpMistakes;
    uint8  profession;   // PlayerBot::BotProfession; 0xFF = not assigned yet
    bool m_dirty;
};

#endif  // EVEMU_PLAYERBOT_BOTMEMORY_H_
