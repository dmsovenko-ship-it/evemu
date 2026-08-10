#ifndef __PLAYER_BOT__H__INCL__
#define __PLAYER_BOT__H__INCL__

#include "npc/NPC.h"
#include "npc/BotMemory.h"
#include "utils/timer.h"
#include <memory>

/**
 * @brief Simulated player — an AI-controlled pilot that populates active systems
 * like a live server. Rides the existing NPC machinery (ship, DestinyMgr,
 * TargetMgr, NPCAI) so movement/combat/modules all work, but carries a fake
 * player "legend" (name/corp/alliance from the agents table) and is driven by
 * a BotMgr decision loop instead of rat aggro logic.
 *
 * @author bot-infrastructure
 */
class PlayerBot
: public NPC
{
public:
    PlayerBot(InventoryItemRef self, EVEServiceManager& services, SystemManager* system, const FactionData& data, uint32 charID, std::string charName, uint32 corpID, uint32 allianceID);
    virtual ~PlayerBot();

    /* SystemEntity interface */
    virtual void Process();
    virtual PyDict* MakeSlimItem();
    virtual void OnAttacked(SystemEntity* attacker);   // assess threat, fight or flee
    virtual void Killed(Damage& damage);               // record death + loss

    /* bot identity */
    uint32 GetBotCharID() const         { return m_botCharID; }
    const std::string& GetBotName() const { return m_botName; }
    uint32 GetBotCorpID() const         { return m_botCorpID; }
    uint32 GetBotAllianceID() const     { return m_botAllianceID; }
    uint8 GetBotSkillLevel() const      { return m_botSkill; }

    /* learning */
    BotMemory* GetMemory() const        { return m_memory.get(); }

    /* bot behaviour control */
    enum class BotActivity : uint8 {
        Idle = 0,       // sitting at a station or drifting
        Traveling,      // warping between gates/stations/anomalies
        Ratting,        // fighting NPCs at an anomaly
        Mining,         // mining an asteroid belt
        Fleeing,        // running from a losing fight
        Docked,         // docked at a station
    };
    void SetActivity(BotActivity act)   { m_activity = act; }
    BotActivity GetActivity() const     { return m_activity; }

    /* travel between systems (via gates) */
    void MarkForTravel();               // bot decides to head for a gate / next system
    bool WantsToTravel() const          { return m_wantsTravel; }
    void ClearTravel()                  { m_wantsTravel = false; }
    bool IsTraveling() const            { return m_traveling; }   // flying to the gate
    void CompleteTravel()               { m_traveling = false; m_wantsTravel = false; }

    /* bot role in combat */
    enum class BotRole : uint8 {
        Fighter = 0,    // pure DPS
        Logistics,      // remote-repair allies
        Support,        // EWAR: web/scram/ECM/paint
        Commander,      // gang bonuses
    };
    BotRole GetRole() const             { return m_role; }
    void SetRole(BotRole r)             { m_role = r; }
    void UseCombatAbilities();          // logistics / EWAR / bonuses during a fight
    // Intelligent target selection: pick the most valuable enemy in a fight
    // (kill call — commanders/logistics/support before fighters), falling back
    // to the attacker. Returns the chosen SystemEntity* (may be attacker).
    SystemEntity* PickPriorityTarget(SystemEntity* attacker);

    /* bot profession — what this pilot does for a living (mirrors its corp) */
    enum class BotProfession : uint8 {
        Hunter = 0,     // PvP pirate: hunts players/bots (kill rights respected)
        RatHunter,      // peaceful PvE: only shoots NPC red crosses (ratting)
        Miner,          // peaceful: mines asteroid belts
        Trader,         // peaceful: shuttles between stations (market activity)
        Courier,        // peaceful: hauls cargo between systems
        Hacker,         // peaceful: runs data/relic sites
    };
    BotProfession GetProfession() const { return m_profession; }
    void SetProfession(BotProfession p) { m_profession = p; }
    bool IsAggressive() const           { return m_profession == BotProfession::Hunter; }
    void DoProfessionActivity();        // mine/trade/courier/hack while not fighting
    void HuntForTarget();               // PvP hunter: find a legal PvP target and engage
    void RatForTarget();                // PvE rat hunter: find an NPC red cross and engage
    void RequestFleetProtection();      // ask corpmate guards to cover this miner/hauler

protected:
    void DecideNextAction();            // BotMgr hook — pick a new activity
    void CallFleetSupport(SystemEntity* attacker);   // same corp/alliance bots join the fight
    static int GetShipClass(uint16 groupID);         // combat-power tier by ship group

    uint32 m_botCharID;
    std::string m_botName;
    uint32 m_botCorpID;
    uint32 m_botAllianceID;
    uint8 m_botSkill;                   // simulated pilot skill tier (0..5)
    BotActivity m_activity;
    BotRole m_role;                     // combat role assigned at spawn
    BotProfession m_profession;         // livelihood (hunter/miner/trader/courier/hacker)
    std::unique_ptr<BotMemory> m_memory;   // persistent learning (win/loss/chat)
    Timer m_decisionTimer;
    Timer m_travelTimer;                // counts down the visible warp to the gate
    bool m_wantsTravel;                 // true when the bot wants to leave via gate
    bool m_traveling;                   // true while the bot visibly warps to the gate
    Timer m_abilityTimer;               // logistics/EWAR/bonus tick
    bool m_inFight;                     // true while fighting (to record outcomes)
};

#endif
