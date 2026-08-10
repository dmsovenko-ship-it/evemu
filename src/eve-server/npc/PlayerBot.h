#ifndef __PLAYER_BOT__H__INCL__
#define __PLAYER_BOT__H__INCL__

#include "npc/NPC.h"
#include "utils/timer.h"

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

    /* bot identity */
    uint32 GetBotCharID() const         { return m_botCharID; }
    const std::string& GetBotName() const { return m_botName; }
    uint32 GetBotCorpID() const         { return m_botCorpID; }
    uint32 GetBotAllianceID() const     { return m_botAllianceID; }
    uint8 GetBotSkillLevel() const      { return m_botSkill; }

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
    Timer m_decisionTimer;
    Timer m_travelTimer;                // counts down the visible warp to the gate
    bool m_wantsTravel;                 // true when the bot wants to leave via gate
    bool m_traveling;                   // true while the bot visibly warps to the gate
};

#endif
