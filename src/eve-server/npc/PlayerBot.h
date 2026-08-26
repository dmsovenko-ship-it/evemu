#ifndef __PLAYER_BOT__H__INCL__
#define __PLAYER_BOT__H__INCL__

#include "npc/NPC.h"
#include "npc/BotMemory.h"
#include "npc/Drone.h"
#include "utils/timer.h"
#include <memory>
#include <vector>

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
    virtual bool IsPlayerBot()                          { return true; }

    /* bot identity */
    uint32 GetBotCharID() const         { return m_botCharID; }
    const std::string& GetBotName() const { return m_botName; }
    uint32 GetBotCorpID() const         { return m_botCorpID; }
    uint32 GetBotAllianceID() const     { return m_botAllianceID; }
    uint8 GetBotSkillLevel() const      { return m_botSkill; }

    /* drone warfare (drone-capable hulls) */
    // Launch up to `count` combat drones around this bot (no ShipSE needed — the
    // bot commands them directly via DestinyMgr). They orbit the bot when idle.
    void SpawnDrones(uint8 count);
    // Called each Process tick: drones orbit the bot when idle, or attack a target
    // (own or an ally's — assist). Reap drones that drifted too far or died.
    void ManageDrones();
    // Return drones to the bot and remove them (scoop / flee / dock).
    void RecallDrones();
    // Direct a drone to engage a target (attack) — used for own target and assist.
    void DroneEngageTarget(DroneSE* drone, SystemEntity* target);
    // How many drones this hull can realistically field (drone bay size).
    uint8 GetDroneCapacity() const;

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
    void MarkForTravel(uint32 destSystem = 0);   // fly to the gate, then cross to destSystem (0 = random)
    bool WantsToTravel() const          { return m_wantsTravel; }
    void ClearTravel()                  { m_wantsTravel = false; }
    bool IsTraveling() const            { return m_traveling; }   // flying to the gate
    void CompleteTravel()               { m_traveling = false; m_wantsTravel = false; }
    uint32 GetTravelDestination() const { return m_destSystemID; }
    void SetTravelDestination(uint32 sysID) { m_destSystemID = sysID; }

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

    /* combat style — how the bot fights once engaged */
    enum class CombatStyle : uint8 {
        Kite = 0,       // keep range, weapons reach, stay out of brawlers' face
        Brawler,        // close in, orbit tight, scrap it out
        Balanced,       // default: orbit at weapon optimal
    };
    CombatStyle GetCombatStyle() const  { return m_combatStyle; }
    void SetCombatStyle(CombatStyle s)  { m_combatStyle = s; }
    // Apply the combat style to the NPC AI (orbit range override).
    void ApplyCombatStyle();

    /* bot profession — what this pilot does for a living (mirrors its corp) */
    enum class BotProfession : uint8 {
        Hunter = 0,     // PvP pirate: hunts players/bots (kill rights respected)
        RatHunter,      // peaceful PvE: only shoots NPC red crosses (ratting)
        Miner,          // peaceful: mines asteroid belts
        Trader,         // peaceful: shuttles between stations (market activity)
        Courier,        // peaceful: hauls cargo between systems
        Hacker,         // peaceful: runs data/relic sites
        Explorer,       // peaceful: scans probes, finds signatures & wormholes
    };
    BotProfession GetProfession() const { return m_profession; }
    void SetProfession(BotProfession p) { m_profession = p; }
    bool IsAggressive() const           { return m_profession == BotProfession::Hunter; }
    void DoProfessionActivity();        // mine/trade/courier/hack while not fighting
    void HuntForTarget();               // PvP hunter: find a legal PvP target and engage
    void RatForTarget();                // PvE rat hunter: find an NPC red cross and engage
    void ClaimSystem();                 // PvP war corps: claim unowned nullsec system (skirmish)
    void RequestFleetProtection();      // ask corpmate guards to cover this miner/hauler
    void ScanForSites();                // explorer: scan probes, find signatures/wormholes
    // Behaviour when the current system has NO station (a pirate/wormhole system,
    // or a waypoint on a route to a hub). Without a station the bot cannot dock,
    // so it does something plausible instead of standing at the gate forever.
    bool HasStationInSystem();    // true if a dockable station exists here
    void PatrolForIdle();               // wander between gates/belts/anomalies (no station)
    void HeadTowardHub(uint32 hubSystem);  // long-range: go to the trade hub
    // Wants to dock (end a mining run, trader works the station, etc.).
    void RequestDock()                  { m_wantsDock = true; }
    void ClearDockRequest()             { m_wantsDock = false; }
    bool WantsDock() const              { return m_wantsDock; }
    // Jump freighter (big cargo / courier contracts): light a cyno, hold an
    // interception window, then jump to the destination system.
    void StartJumpFreighter(uint32 destSystem);
    bool IsJumpFreighter() const        { return m_isJumpFreighter; }
    bool CynoActive() const             { return m_cynoActive; }

    /* realistic PvP behaviour (evaluate before engaging, mistakes for novices) */
    bool IsNearGate(double threshold = 60000.0);   // within X m of a gate (ambush risk)
    int  CountEnemiesNearby(SystemEntity* target, double radius = 100000.0);  // hostile ships around target
    int  CountAlliesNearby(double radius = 100000.0);                          // friendly ships around me
    bool ShouldEngage(int myPower, int theirPower, bool defending);   // power + skill + luck decision
    // A hunter's decision to commit against a REAL player (not a bot): strength
    // check + skill-based confidence. Used by the NPCAI Idle scan so hunters
    // occasionally prowl for players without the whole crowd jumping them.
    bool HunterWouldEngage(SystemEntity* target);
    // Faction of the bot's starter corp: 4=Amarr, 2=Minmatar, 1=Caldari, 8=Gallente,
    // 0=neutral. Stub for a future "faction warfare" bot subclass (faction war =
    // just a flavour of hunting with fixed enemies). Normal bots ignore it.
    int GetFaction() const;
    bool IsFactionEnemy(const PlayerBot* other) const;
    // Faction-warrior flag: a hunter subclass that treats bots of OTHER factions
    // as its fixed enemies (like FW militia). Set for a minority of hunters.
    void SetFactionWarrior(bool w)      { m_factionWarrior = w; }
    bool IsFactionWarrior() const       { return m_factionWarrior; }
    // Update standings between bots after a fight: the loser's pilot and corp
    // take a standings hit from the winner's side (grudge), the winner gains a
    // bit. Corp-level too, so whole corps slowly become enemies — the basis for
    // future faction/aggression tracking by standings.
    void UpdateBotStandings(const PlayerBot* other, bool otherLost);
    // Aggression timer: after the bot attacks someone it can't dock or jump a
    // gate for a while (like a real pilot's aggression flag). Start() it on an
    // attack; IsAggressed() gates docking/travel in BotMgr.
    void StartAggressionTimer()   { m_aggressionTimer.Start(MakeRandomInt(30000, 90000)); }
    bool IsAggressed() const      { return m_aggressionTimer.Enabled(); }
    // Broadcast an OnAggressionChange notification so players in the bubble see
    // the bot's blinking aggression icon (like any attacker's). victim = who the
    // bot attacked (may be a Client or another bot).
    void BroadcastAggression(uint32 victimCharID);
    // Advanced hunter ambush: drop a warp bubble to trap the target, call the
    // fleet, then engage. Called from NPCAI's Idle scan (player targets) and
    // HuntForTarget (bot prey).
    bool TryAmbush(SystemEntity* target);

protected:
    void DecideNextAction();            // BotMgr hook — pick a new activity
    void CallFleetSupport(SystemEntity* attacker);   // same corp/alliance bots join the fight
    static int GetShipClass(uint16 groupID);         // combat-power tier by ship group
    static bool IsCombatHull(uint16 groupID);       // hull that can actually fight
    void AnalyzeCombatSituation();                  // re-target priority + disengage check (runs during fights)
    void DeployWarpBubble(const GPoint& pos);       // drop a Mobile Warp Disruptor bubble at pos
    void RecordPvpOutcome(bool won);    // win/loss + PvP judgement learning

    uint32 m_botCharID;
    std::string m_botName;
    uint32 m_botCorpID;
    uint32 m_botAllianceID;
    uint8 m_botSkill;                   // simulated pilot skill tier (0..5)
    BotActivity m_activity;
    BotRole m_role;                     // combat role assigned at spawn
    CombatStyle m_combatStyle;          // kite / brawler / balanced (assigned at spawn)
    BotProfession m_profession;         // livelihood (hunter/miner/trader/courier/hacker)
    std::unique_ptr<BotMemory> m_memory;   // persistent learning (win/loss/chat)
    Timer m_decisionTimer;
    uint32 m_decisionCount;             // number of decisions made — first one fires fast after spawn
    Timer m_travelTimer;                // counts down the visible warp to the gate
    bool m_wantsTravel;                 // true when the bot wants to leave via gate
    bool m_traveling;                   // true while the bot visibly warps to the gate
    uint32 m_destSystemID;              // target system to cross into (0 = random)
    Timer m_abilityTimer;               // logistics/EWAR/bonus tick
    Timer m_activityTimer;              // profession run counter (self-learning)
    Timer m_huntCooldown;               // PvP hunter: pause between engages (no gate camping)
    Timer m_scoutTimer;                 // fresh arrival: scout the system before committing
    Timer m_aggressionTimer;            // aggression flag: can't dock/jump while active
    bool m_inFight;                     // true while fighting (to record outcomes)
    bool m_wantsDock;                   // true when the bot wants to dock (profession)
    uint8 m_mineTrips;                  // mining runs since last dock (ore haul)
    bool m_isJumpFreighter;             // flying a jump freighter (big cargo)
    bool m_cynoActive;                  // cyno is lit — interception window open
    bool m_factionWarrior;              // FW subclass: fights bots of other factions
    uint32 m_jumpDest;                  // destination system for the jump
    Timer m_cynoTimer;                  // window before the jump fires
    std::vector<DroneSE*> m_drones;     // launched combat drones (bot commands directly)
    Timer m_droneTimer;                 // drone attack cycle
};

#endif
