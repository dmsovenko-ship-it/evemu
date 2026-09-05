#include "eve-server.h"
#include "EVEServerConfig.h"
#include "npc/PlayerBot.h"
#include "npc/BotMgr.h"
#include "npc/NPCAI.h"
#include "npc/Drone.h"
#include "system/Damage.h"
#include "ship/Ship.h"
#include "Client.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/BeltMgr.h"
#include "system/Asteroid.h"
#include "system/Container.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/sov/SovereigntyDataMgr.h"
#include "standing/StandingDB.h"
#include "ServiceDB.h"
#include "StaticDataMgr.h"
#include <iterator>
#include <cmath>
#include <sstream>

/*
 * @file PlayerBot.cpp
 *
 * Simulated player — rides the NPC machinery but behaves like a real pilot.
 * BotMgr drives it: picks a system, spawns it (through a gate from the
 * neighbouring system for believability), lets it travel/dock/rat/mine,
 * and reaps it when the system goes quiet.
 */

PlayerBot::PlayerBot(InventoryItemRef self, EVEServiceManager& services, SystemManager* system, const FactionData& data, uint32 charID, std::string charName, uint32 corpID, uint32 allianceID)
: NPC(self, services, system, data, nullptr),
  m_botCharID(charID),
  m_botName(std::move(charName)),
  m_botCorpID(corpID),
  m_botAllianceID(allianceID),
  m_botSkill(3),
  m_activity(BotActivity::Idle),
  m_role(BotRole::Fighter),
  m_combatStyle(CombatStyle::Balanced),
  m_profession(BotProfession::Miner),
  m_memory(std::make_unique<BotMemory>(charID)),
  m_decisionTimer(0),
  m_decisionCount(0),
  m_travelTimer(0),
  m_wantsTravel(false),
  m_traveling(false),
  m_destSystemID(0),
  m_abilityTimer(0),
  m_activityTimer(0),
  m_inFight(false),
  m_wantsDock(false),
  m_mineTrips(0),
  m_isJumpFreighter(false),
  m_cynoActive(false),
  m_factionWarrior(false),
  m_jumpDest(0),
  m_cynoTimer(0),
  m_fleetBoss(false)
{    // A player-like legend: give this NPC a neutral alliance so it doesn't show
    // red crosshairs and isn't auto-aggroed by faction standing checks. The
    // owner is the PILOT (charID), not the corp — so clients can lock the ship
    // and show its pilot as the owner.
    m_allyID = m_botAllianceID;
    m_corpID = m_botCorpID;
    m_warID = 0;
    m_ownerID = m_botCharID;

    // Load persistent learning (win/loss history, chat quality).
    m_memory->Load();
}

PlayerBot::~PlayerBot() = default;

PyDict* PlayerBot::MakeSlimItem()
{
    PyDict* slim = NPC::MakeSlimItem();
    if (slim != nullptr) {
        // Simulated player: neutral security status, real player-style identity.
        slim->SetItemString("name", new PyString(m_botName));
        slim->SetItemString("corpID", new PyInt(m_botCorpID));
        slim->SetItemString("allianceID", new PyInt(m_botAllianceID));
        // Security status reflects the bot's history: a veteran killer shows a
        // red skull (< -5.0), a brawler an orange badge (-5..-0.1), a clean pilot
        // 0.0+. Kills lower it, deaths/non-pvp don't raise it much.
        float sec = 0.0f;
        if (m_memory != nullptr) {
            int kills = (int)m_memory->GetKills();
            if (kills > 0)
                sec = -0.5f - (float)kills * 0.9f;   // each kill ≈ -0.9
            // Cap: heavy killers deep into criminal range (red skull).
            if (sec < -10.0f) sec = -10.0f;
            // A bot that died a lot is a bad pilot, slightly negative but not criminal.
            if (kills == 0 && m_memory->GetDeaths() > 3)
                sec = -0.3f;
        }
        slim->SetItemString("securityStatus", new PyFloat(sec));
        slim->SetItemString("characterID", new PyInt(m_botCharID));
        // A charbot is a ship with a pilot: charID must be set (not None like
        // plain NPCs) or the client doesn't treat it as a piloted ship — camera
        // follow ("Смотреть за") and pilot identity rely on this field.
        slim->SetItemString("charID", new PyInt(m_botCharID));
    }
    return slim;
}

void PlayerBot::OnAttacked(SystemEntity* attacker)
{
    if (m_killed || attacker == nullptr || m_destiny == nullptr)
        return;

    // A bot is a neutral pilot: it doesn't gank, but it will defend itself.
    // First check the RIGHT to kill — a careful pilot won't take CONCORD in
    // highsec for no reason. Rules mirror EVE:
    //   highsec (>=0.5): may attack only if the target is already a criminal or
    //                    has a very low security status (legal target);
    //   lowsec  (0.1-0.4): almost free (only station guns are a concern);
    //   nullsec (<0.1):     no restrictions at all.
    float sysSec = SystemMgr() != nullptr ? SystemMgr()->GetSystemSecurityRating() : 0.0f;
    bool mayAttack = true;
    if (sysSec >= 0.5f) {
        bool targetCriminal = false;
        bool targetLowSec = false;
        if (attacker->HasPilot()) {
            Client* atk = attacker->GetPilot();
            if (atk->GetCrimeWatch() != nullptr)
                targetCriminal = atk->GetCrimeWatch()->IsCriminal();
            targetLowSec = atk->GetSecurityRating() < -5.0f;
        }
        mayAttack = targetCriminal || targetLowSec;
    }

    // Evaluate the fight: compare its own combat power (skill tier + ship class)
    // to the attacker's ship class. If it thinks it can win (or the fight is
    // even), it fights back; otherwise it warps out.
    int attackerClass = 0;
    if (attacker->GetSelf().get() != nullptr) {
        Inv::TypeData tdata;
        sDataMgr.GetType(attacker->GetSelf()->typeID(), tdata);
        attackerClass = GetShipClass(tdata.groupID);
    }
    int myClass = GetShipClass(m_self->groupID());

    int myPower = myClass * 2 + (int)m_botSkill;
    int theirPower = attackerClass * 2;
    theirPower += CountEnemiesNearby(attacker) * 3;   // friends of the attacker add to its strength
    myPower += CountAlliesNearby() * 2;               // my fleet helps me

    // Analytic threat (never a precise fit-check — a pilot can't read the other
    // fit). Capitals can cyno in a fleet / rarely travel alone; battleships are
    // assumed fitted to fight; a hull that has fighters/drones out in space is
    // a real carrier screen, not just a class guess.
    using namespace EVEDB::invGroups;
    uint16 atkGroup = 0;
    if (attacker->GetSelf().get() != nullptr) {
        Inv::TypeData atkData;
        sDataMgr.GetType(attacker->GetSelf()->typeID(), atkData);
        atkGroup = atkData.groupID;
    }
    if (atkGroup == Carrier || atkGroup == Supercarrier || atkGroup == Titan)
        theirPower += 4;   // may cyno in friends / bring a fleet
    else if (atkGroup == Battleship || atkGroup == BlackOps || atkGroup == Marauder)
        theirPower += 1;   // heavy hull — assume it's fitted to fight
    if (attacker->GetDroneSE() != nullptr
        || (attacker->IsShipSE() && attacker->GetShipSE()->GetActiveFighterCount() > 0))
        theirPower += 3;   // fighter screen / drones already out

    // A real player is more dangerous than an AI pilot of the same hull — bots
    // defend cautiously against players but brawl more readily with each other.
    if (attacker->HasPilot())
        theirPower += 2;   // cautious vs players
    else if (attacker->GetNPCSE() != nullptr)
        theirPower -= 1;   // bolder vs bots

    // "Bait": a hunter in a weak-looking hull deliberately lets the enemy commit
    // (they think it's an easy kill), then the fleet warps in. The bait holds the
    // attacker with a scram and calls support instead of fleeing.
    bool isBait = (m_profession == BotProfession::Hunter
                   && m_role == BotRole::Fighter
                   && myClass <= 2 && MakeRandomInt(0, 99) < 25);

    // A non-combat hull (industrial, mining barge, freighter, hauler...) has no
    // real guns — a real pilot in one never fights back, it warps out. Keeps a
    // courier's Badger from "defending" against a supercarrier.
    if (!IsCombatHull(m_self->groupID())) {
        _log(BOT__TRACE, "PlayerBot %s(%u): non-combat hull — fleeing instead of fighting back.",
             m_botName.c_str(), m_botCharID);
        RecallDrones();
        GetAIMgr()->StartAttackCycle(0);
        GetAIMgr()->Flee(attacker);
        return;
    }

    // Any bot defends itself when attacked (self-defence is legal and natural) —
    // a miner on a Retriever has no guns so it just flees, but a peaceful pilot on
    // a combat hull fights back. What they never do is initiate the fight: only
    // Hunters/RatHunters seek targets (HuntForTarget/RatForTarget).
    if (mayAttack && (isBait || ShouldEngage(myPower, theirPower, true))) {
        // Legal and confident — fight back (NPCAI handles targeting/attack).
        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
        ApplyCombatStyle();
        GetAIMgr()->WakeUp();
        GetAIMgr()->StartAttackCycle(2000);
        // Fighting back = aggression. The bot can't dock or jump until it cools
        // down — a real pilot can't just leave a fight and dock.
        StartAggressionTimer();
        // Drone hulls field their drones when combat starts.
        if (GetDroneCapacity() > 0 && m_drones.empty())
            SpawnDrones(0);
        if (attacker->HasPilot())
            BroadcastAggression(attacker->GetPilot()->GetCharacterID());
        else if (attacker->GetNPCSE() != nullptr) {
            PlayerBot* enemyBot = dynamic_cast<PlayerBot*>(attacker->GetNPCSE());
            if (enemyBot != nullptr)
                BroadcastAggression(enemyBot->GetBotCharID());
        }
        // Tackle: hold the attacker so it can't warp while the fleet arrives.
        // NPCAI's AttackTarget applies the scram once in range.
        if (GetAIMgr()->GetScramRange() <= 0)
            GetAIMgr()->SetScram(15000.0f, 1, 0.6f);
        // Record the grudge: being attacked and fighting back means the attacker
        // is an enemy — set standings (personal + corp). If it's another bot.
        if (attacker->GetNPCSE() != nullptr) {
            PlayerBot* enemyBot = dynamic_cast<PlayerBot*>(attacker->GetNPCSE());
            if (enemyBot != nullptr)
                UpdateBotStandings(enemyBot, false);   // I'm the victim this round
        }
        // Intelligent fleet support: call allies (same corp or same alliance)
        // in this system to join the fight, so fights are decided by force
        // concentration, not one brave pilot.
        CallFleetSupport(attacker);
    } else {
        // Not legal to fight, or outmatched — flee. A professional pilot warps
        // out rather than suicide against a superior (or unlawful) force.
        _log(BOT__TRACE, "PlayerBot %s(%u): fleeing (%s).",
             m_botName.c_str(), m_botCharID, mayAttack ? "outmatched" : "no kill right");
        RecallDrones();   // scoop drones before warping out
        GetAIMgr()->StartAttackCycle(0);
        GetAIMgr()->Flee(attacker);
    }
}

void PlayerBot::CallFleetSupport(SystemEntity* attacker)
{
    if (attacker == nullptr || SystemMgr() == nullptr)
        return;
    // Rally allies from the same corp OR the same alliance that are in this
    // system. They join the fight and pick a priority target (kill call), so a
    // fleet concentrates fire on the most valuable enemy — mirroring real EVE.
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        if (se == this)
            continue;
        PlayerBot* ally = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (ally == nullptr)
            continue;
        if (ally->GetBotCorpID() != m_botCorpID && ally->GetBotAllianceID() != m_botAllianceID)
            continue;   // not same corp / alliance — not an ally
        // Don't call a hauler/barge/freighter into a fight — non-combat hulls
        // can't contribute and would just die (or sit there looking silly).
        if (!IsCombatHull(ally->GetSelf()->groupID()))
            continue;
        // Only rally if the ally isn't already busy fighting or fleeing.
        if (ally->GetAIMgr()->IsFighting())
            continue;
        _log(BOT__TRACE, "PlayerBot %s(%u): fleet support — %s(%u) joining.",
             m_botName.c_str(), m_botCharID, ally->GetBotName().c_str(), ally->GetBotCharID());
        ally->GetAIMgr()->WakeUp();
        ally->GetAIMgr()->StartAttackCycle(2000);
        SystemEntity* prio = ally->PickPriorityTarget(attacker);
        ally->GetAIMgr()->Target(prio != nullptr ? prio : attacker);
    }
}

// Choose the most valuable enemy currently in a fight with us: commanders and
// logistics die first (they keep the enemy fleet alive / buffed), then EWAR
// support, then plain fighters. If we can't tell, fall back to the attacker.
SystemEntity* PlayerBot::PickPriorityTarget(SystemEntity* attacker)
{
    SystemEntity* best = attacker;
    int bestScore = -1;

    // Consider enemy PlayerBots in this system that are fighting.
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* enemy = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (enemy == nullptr)
            continue;
        if (enemy == this)
            continue;
        if (enemy->GetBotCorpID() == m_botCorpID || enemy->GetBotAllianceID() == m_botAllianceID)
            continue;   // ally, not target
        if (!enemy->GetAIMgr()->IsFighting())
            continue;   // not engaged — don't pull aggro

        // Score by role value: commanders/logistics most, then support, fighters last.
        int score = 0;
        switch (enemy->GetRole()) {
            case BotRole::Commander:   score = 4; break;
            case BotRole::Logistics:   score = 3; break;
            case BotRole::Support:     score = 2; break;
            default:                   score = 1; break;
        }
        // Prefer targets in range (don't chase across the system).
        if (GetPosition().distance(enemy->GetPosition()) > 60000)
            score -= 2;
        if (score > bestScore) { bestScore = score; best = enemy; }
    }
    return best;
}

// Ship class by groupID: bigger index = more combat power.
// Non-combat hulls (mining barges, industrials, freighters, shuttles...) are 0 —
// a pilot in one is defenceless and never wins a fight, so power assessments
// (ShouldEngage/HunterWouldEngage) always make them flee.
int PlayerBot::GetShipClass(uint16 groupID)
{
    using namespace EVEDB::invGroups;
    switch (groupID) {
        case Frigate: case AssaultShip: case Interceptor:
        case CovertOps: case Interdictor: case StealthBomber:
            return 1;
        case Destroyer:
            return 2;
        case Cruiser: case HeavyAssaultShip: case CombatRecon:
        case Logistics:
            return 3;
        case Battlecruiser: case CommandShip: case StrategicCruiser:
            return 4;
        case Battleship: case BlackOps: case Marauder:
            return 5;
        case Supercarrier: case Titan: case Carrier:
            return 6;
        // no real weapons — never wins a straight fight
        case MiningBarge: case Exhumer: case Industrial:
        case Freighter: case TransportShip: case JumpFreighter:
        case CapitalIndustrialShip: case Shuttle: case Capsule:
        case IndustrialCommandShip:
            return 0;
        default:
            return 2;   // unknown — treat as cruiser-ish
    }
}

// A hull that can actually fight. Industrials, mining barges/exhumers,
// freighters, shuttles etc. carry no real weapons — a pilot in one never fights
// back, it warps out. Used to keep haulers/barges out of combat entirely.
bool PlayerBot::IsCombatHull(uint16 groupID)
{
    using namespace EVEDB::invGroups;
    switch (groupID) {
        case Frigate: case AssaultShip: case Interceptor:
        case CovertOps: case Interdictor: case StealthBomber:
        case Destroyer:
        case Cruiser: case HeavyAssaultShip: case CombatRecon:
        case Logistics:
        case Battlecruiser: case CommandShip: case StrategicCruiser:
        case Battleship: case BlackOps: case Marauder:
        case Supercarrier: case Titan: case Carrier:
            return true;
        default:
            return false;   // industrial, mining barge, exhumers, freighter, shuttle...
    }
}

void PlayerBot::MarkForTravel(uint32 destSystem /*0*/)
{
    if (m_traveling || m_wantsTravel)
        return;
    if (destSystem != 0)
        m_destSystemID = destSystem;
    // Visibly warp to a gate in this system. The travel timer covers the flight
    // time (a few AU at warp speed); when it expires the bot is gone from here.
    // The bot does NOT get deleted at the moment it decides — it first flies.
    m_traveling = true;
    m_travelTimer.Start(MakeRandomInt(12000, 20000));   // visible warp to gate

    // Warp toward a random gate in the system so the player sees the ship leave.
    // Land OUTSIDE the gate's collision sphere (gates are 14-19km radius; warping
    // to the centre or 1000m out leaves the ship inside it, so collision snaps it
    // around every tick = teleports). Warp to a point past the gate's surface.
    auto gates = SystemMgr()->GetGates();
    if (!gates.empty()) {
        auto it = gates.begin();
        std::advance(it, MakeRandomInt(0, (int64)gates.size() - 1));
        if (it->second != nullptr) {
            GPoint gatePos = it->second->GetPosition();
            double gateR = it->second->GetRadius() > 500.0 ? it->second->GetRadius() : 3000.0;
            GPoint target = gatePos + GPoint(gateR + 15000.0, 0, 0);
            DestinyMgr()->WarpTo(target, 0);
            _log(BOT__TRACE, "PlayerBot %s(%u): warping to gate for departure.",
                 m_botName.c_str(), m_botCharID);
        }
    }
    // NOTE: m_wantsTravel is set by Process() when the warp-to-gate flight finishes
    // (m_travelTimer expiry). Setting it here too made ProcessTravel() delete the bot
    // on the very next tic — the bot vanished instantly without the visible ~12-20s
    // warp to the gate ("a chelobot arrived, judged the fight and just disappeared").
}

void PlayerBot::Process()
{
    // Rides NPC::Process for movement/destiny/target handling, but the bot's
    // activities (travel, dock, rat, mine, flee) are orchestrated by BotMgr,
    // which calls DecideNextAction() and issues Destiny commands directly.
    NPC::Process();

    // Manage launched drones: orbit while idle, attack during combat (or assist
    // an ally), and scoop drones that drift too far.
    ManageDrones();

    // When the visible warp-to-gate flight is done, signal BotMgr we're ready
    // to actually cross the gate. Until then we stay here, visibly flying.
    if (m_traveling && m_travelTimer.Check(false))
        m_wantsTravel = true;

    // During a fight, use role abilities (logistics, EWAR, gang bonuses).
    bool fighting = GetAIMgr()->IsFighting();
    if (fighting && m_abilityTimer.Check(false))
        UseCombatAbilities();
    if (!m_abilityTimer.Enabled())
        m_abilityTimer.Start(5000);

    // Learn from fights: when a fight ends and we survived, count it as a win.
    if (m_inFight && !fighting && !m_killed) {
        m_inFight = false;
        if (m_memory) { m_memory->RecordWin(); m_memory->Save(); }
        // PvE rat hunter also learns from a rat kill, and scoops the wreck it
        // made (tractor + salvage) so its hold fills with real loot.
        if (m_profession == BotProfession::RatHunter && m_memory) {
            m_memory->RecordRatKill();
            m_memory->Save();
            SalvageMyWrecks();
        }
        // A surviving fight is practice — check for a skill level-up.
        LevelUpFromPractice();
        _log(BOT__TRACE, "PlayerBot %s(%u): fight ended, recorded win.", m_botName.c_str(), m_botCharID);
    } else if (fighting) {
        m_inFight = true;
    }

    // Profession learning: each completed activity run adds experience so the bot
    // gets better at its job (see GetActivitySkill). Enough practice also raises
    // its actual skill tier (see LevelUpFromPractice) — a veteran pilot.
    if (m_profession != BotProfession::Hunter && m_profession != BotProfession::RatHunter
        && m_activityTimer.Check(false) && m_memory != nullptr)
    {
        switch (m_profession) {
            case BotProfession::Miner:     m_memory->RecordMineRun(); break;
            case BotProfession::Trader:    m_memory->RecordTradeRun(); break;
            case BotProfession::Courier:   m_memory->RecordTradeRun(); break;
            case BotProfession::Hacker:    m_memory->RecordHackRun(); break;
            default: break;
        }
        m_memory->Save();
        LevelUpFromPractice();
        _log(BOT__TRACE, "PlayerBot %s(%u): recorded profession run.", m_botName.c_str(), m_botCharID);
    }
    if (!m_activityTimer.Enabled())
        m_activityTimer.Start(120000);   // a profession "run" every ~2 min

    // Jump freighter cyno phase: when the window expires, jump to the
    // destination (JumpDrive effects), complete the haul.
    if (m_cynoActive && m_cynoTimer.Check(false)) {
        m_cynoActive = false;
        _log(BOT__MESSAGE, "PlayerBot %s(%u): jump freighter jumping to system %u.",
             m_botName.c_str(), m_botCharID, m_jumpDest);
        if (m_destiny != nullptr)
            m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                         m_self->itemID(), 0, "effects.JumpDriveOut",
                                         1, 1, 1, 3000, 0, 0);
        // Hand off to BotMgr to move the freighter to the destination.
        SetTravelDestination(m_jumpDest);
        m_wantsTravel = true;
    }

    // When not fighting or traveling, do the bot's profession activity
    // (mine/trade/courier/hack). Hunters prowl for targets instead.
    if (!fighting && !m_traveling && !m_wantsTravel && m_decisionTimer.Check(false)) {
        DoProfessionActivity();
    }

    if (m_decisionTimer.Check(false)) {
        DecideNextAction();
    }
    if (!m_decisionTimer.Enabled()) {
        // Decision cadence depends on system security. In lowsec/nullsec a pilot
        // reacts fast — sitting idle at a gate is how you get ganked — so the bot
        // makes up its mind within a few seconds. In highsec it can be leisurely.
        // A freshly spawned bot decides almost immediately instead of standing at
        // the gate for the old flat 15s before its first move.
        float sec = SystemMgr() != nullptr ? SystemMgr()->GetSystemSecurityRating() : 1.0f;
        uint32 delayMs;
        if (m_decisionCount == 0)
            delayMs = MakeRandomInt(2, 5) * 1000;           // first action right after arrival
        else if (sec < 0.45f)
            delayMs = MakeRandomInt(4, 9) * 1000;           // lowsec / nullsec — act quickly
        else
            delayMs = MakeRandomInt(12, 25) * 1000;         // highsec — no hurry
        ++m_decisionCount;
        m_decisionTimer.Start(delayMs);
    }
}

void PlayerBot::Killed(Damage& damage)
{
    // Record the loss for learning, then let the base NPC clean up.
    if (m_memory) { m_memory->RecordLoss(); m_memory->RecordDeath(); m_memory->Save(); }
    RecallDrones();   // drones are lost/recalled with the ship
    // The killer (if a bot) has proven itself an enemy — deep grudge, both ways.
    if (damage.srcSE != nullptr && damage.srcSE->GetNPCSE() != nullptr) {
        PlayerBot* killer = dynamic_cast<PlayerBot*>(damage.srcSE->GetNPCSE());
        if (killer != nullptr)
            UpdateBotStandings(killer, true);   // other (killer) won, I lost
    }

    // Chelobots are real pilots as far as the killboard is concerned — record a
    // killmail so their ship appears in chrKillTable (like any player loss).
    RecordBotKillMail(damage);

    NPC::Killed(damage);
}

// Build + persist a killmail for a destroyed chelobot and notify the killer.
void PlayerBot::RecordBotKillMail(Damage& fatal_blow)
{
    if (m_system == nullptr || m_self.get() == nullptr)
        return;

    SystemEntity* killer = fatal_blow.srcSE;
    if (killer == nullptr)
        return;

    // resolve the killer (player, player drone, NPC, or another chelobot)
    uint32 killerID = 0;
    Client* pClient = nullptr;
    if (killer->HasPilot()) {
        pClient = killer->GetPilot();
        if (pClient != nullptr) killerID = pClient->GetCharacterID();
    } else if (killer->IsDroneSE()) {
        Client* owner = killer->GetDroneSE() != nullptr ? killer->GetDroneSE()->GetOwner() : nullptr;
        if (owner != nullptr) { pClient = owner; killerID = owner->GetCharacterID(); }
    } else {
        killerID = killer->GetCorporationID();
        if (killerID == 0) killerID = killer->GetID();
    }

    KillData data = KillData();
    data.solarSystemID = m_system->GetID();
    data.victimCharacterID = GetBotCharID();
    data.victimCorporationID = GetBotCorpID();
    data.victimAllianceID = GetBotAllianceID();
    data.victimFactionID = GetWarFactionID();
    data.victimShipTypeID = GetTypeID();

    data.finalCharacterID = killerID;
    // Fighter/drone final blow → report the PILOT's ship/corp, drone as the weapon.
    uint16 finalShipTypeID = killer->GetTypeID();
    int32 finalCorpID = killer->GetCorporationID();
    int32 finalAllyID = killer->GetAllianceID();
    if (pClient != nullptr) {
        finalCorpID = pClient->GetCorporationID();
        finalAllyID = pClient->GetAllianceID();
        ShipSE* kShip = pClient->GetShipSE();
        if (kShip != nullptr)
            finalShipTypeID = kShip->GetTypeID();
    }
    data.finalCorporationID = finalCorpID;
    data.finalAllianceID = finalAllyID;
    data.finalFactionID = (pClient != nullptr ? pClient->GetWarFactionID()
                          : (killer->GetWarFactionID() > 500021 ? 500021 : killer->GetWarFactionID()));
    data.finalShipTypeID = finalShipTypeID;
    data.finalWeaponTypeID = (fatal_blow.weaponRef.get() != nullptr) ? fatal_blow.weaponRef->typeID() : killer->GetTypeID();
    data.finalSecurityStatus = (pClient != nullptr) ? pClient->GetSecurityRating() : 0.0;
    data.finalDamageDone = static_cast<uint32>(fatal_blow.GetTotal());

    uint32 totalHP = m_self->GetAttribute(AttrHP).get_int();
    totalHP += m_self->GetAttribute(AttrArmorHP).get_int();
    totalHP += m_self->GetAttribute(AttrShieldCapacity).get_int();
    data.victimDamageTaken = totalHP;

    // dropped/destroyed items (modules + cargo from the ship)
    {
        std::stringstream blob;
        blob << "<items>";
        bool foundItems = false;
        uint32 shipItemID = m_self->itemID();
        DBQueryResult iRes;
        if (sDatabase.RunQuery(iRes,
            "SELECT typeID, flag, quantity, singleton FROM entity WHERE locationID = %u", shipItemID)) {
            DBResultRow irow;
            while (iRes.GetRow(irow)) {
                foundItems = true;
                uint32 typeID = irow.GetUInt(0);
                uint32 flag   = irow.GetUInt(1);
                uint32 qty    = irow.GetUInt(2);
                uint32 single = irow.GetUInt(3);
                uint32 d = 0, x = qty;
                if (IsRigSlot(flag) || IsSubSystem(flag)) {
                    // rigs/subs destroyed
                } else if (IsEven(MakeRandomInt(0, 100))) {
                    if (qty > 1) { d = MakeRandomInt(0, qty); x = qty - d; }
                }
                blob << "<i t=" << typeID << " f=" << flag << " q=" << qty << " s=" << single << " d=" << d << " x=" << x << "/>";
            }
        }

        // Chelobots carry no fitted module ITEMS (their weapon is an attribute),
        // so a killmail would list only the hull. Synthesize a believable fit from
        // the hull's known weapon + a generic mid/low set so the kill page shows
        // slots/cargo like any real player kill. No DB items are created — purely
        // cosmetic for the killmail blob.
        if (!foundItems) {
            auto emit = [&](uint32 typeID, uint32 flag) {
                uint32 d = 0, x = 1;
                if (!IsRigSlot(flag) && !IsSubSystem(flag) && IsEven(MakeRandomInt(0, 100)))
                    d = 1, x = 0;
                blob << "<i t=" << typeID << " f=" << flag << " q=1 s=1 d=" << d << " x=" << x << "/>";
            };
            // High slot(s): the weapon this bot actually fires (or a miner laser).
            if (m_self->HasAttribute(AttrGfxTurretID)) {
                uint32 weapon = m_self->GetAttribute(AttrGfxTurretID).get_int();
                if (weapon > 0 && weapon != GetTypeID())
                    emit(weapon, EVEItemFlags::flagHiSlot0);
            }
            // Mid slots.
            emit(439, EVEItemFlags::flagMidSlot0);        // 1MN Afterburner I
            emit(377, EVEItemFlags::flagMidSlot1);        // Small Shield Extender I
            // Low slot.
            emit(2046, EVEItemFlags::flagLowSlot0);       // Damage Control I
        }
        blob << "</items>";
        data.killBlob = blob.str();
    }

    data.killTime = GetFileTimeNow();
    data.moonID = m_system->GetID();

    ServiceDB::SaveKillOrLoss(data);

    // notify a real player killer
    if (pClient != nullptr) {
        std::string secStr = std::to_string(m_system->GetSystemSecurityRating());
        size_t dot = secStr.find('.');
        if (dot != std::string::npos) secStr = secStr.substr(0, dot + 2);
        std::string km;
        km += "Victim: " + GetBotName() + ", Corp: " + sDataMgr.GetOwnerName(GetBotCorpID()) + "\n";
        km += "System: " + std::string(m_system->GetName()) + " (" + secStr + ")\n";
        km += "Damage Taken: " + std::to_string(data.victimDamageTaken) + "\n\n";
        km += "Final Blow: " + std::string(pClient->GetName()) + " flying " + std::string(sDataMgr.GetTypeName(data.finalShipTypeID)) + "\n";
        km += "Damage Done: " + std::to_string(data.finalDamageDone) + "\n";
        pClient->SendNotifyMsg("Kill: %s (%s) - %s (%s) - %u damage",
            GetBotName().c_str(), sDataMgr.GetTypeName(data.victimShipTypeID),
            pClient->GetName(), sDataMgr.GetTypeName(data.finalShipTypeID), data.victimDamageTaken);
        pClient->SelfEveMail("Kill Report", km.c_str());
    }
}

void PlayerBot::RecordPvpOutcome(bool won)
{
    if (m_memory == nullptr)
        return;
    if (won) { m_memory->RecordWin(); m_memory->RecordKill(); }
    else     { m_memory->RecordLoss(); m_memory->RecordDeath(); }
    m_memory->Save();
}

bool PlayerBot::IsNearGate(double threshold)
{
    if (SystemMgr() == nullptr)
        return false;
    for (auto& [id, se] : SystemMgr()->GetStaticEntities()) {
        if (se != nullptr && se->GetGateSE() != nullptr) {
            if (GetPosition().distance(se->GetPosition()) < threshold)
                return true;
        }
    }
    return false;
}

int PlayerBot::CountEnemiesNearby(SystemEntity* target, double radius)
{
    // How many hostile ships are around `target` (the potential victim): other
    // PlayerBots from a different corp/alliance plus real players. A target with
    // friends nearby is a trap — a real hunter warps out instead of ganking.
    int count = 0;
    if (SystemMgr() == nullptr || target == nullptr)
        return 0;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr)
            continue;
        if (se == this || se == target)
            continue;
        if (target->GetPosition().distance(se->GetPosition()) > radius)
            continue;
        if (se->GetNPCSE() != nullptr) {
            PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (other == nullptr)
                continue;
            if (other->GetBotCorpID() == m_botCorpID || other->GetBotAllianceID() == m_botAllianceID)
                continue;   // ally of mine — not an enemy
            ++count;
        } else if (se->GetPilot() != nullptr) {
            ++count;   // real player near the target is a friend of the target
        }
    }
    return count;
}

int PlayerBot::CountAlliesNearby(double radius)
{
    int count = 0;
    if (SystemMgr() == nullptr)
        return 0;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se == this)
            continue;
        if (GetPosition().distance(se->GetPosition()) > radius)
            continue;
        if (se->GetNPCSE() != nullptr) {
            PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (other == nullptr)
                continue;
            if (other->GetBotCorpID() == m_botCorpID || other->GetBotAllianceID() == m_botAllianceID)
                ++count;
        }
    }
    return count;
}

bool PlayerBot::ShouldEngage(int myPower, int theirPower, bool defending)
{
    // Core aggression gate. A bot does NOT attack everyone it meets — it judges
    // the fight like a real pilot: needs a clear edge, no nearby friends of the
    // target, and only a practised bot is confident. Self-learning moves the
    // line: winners grow bolder, losers grow cautious.
    if (myPower <= 0 || theirPower <= 0)
        return false;

    // PvP skill (0..1): novices misjudge, veterans are reliable.
    float skill = m_memory ? m_memory->GetPvpSkill() : 0.0f;
    // AggroFactor from config: positive = bolder, negative = careful.
    float aggro = (float)sConfig.playerBots.AggroFactor / 100.0f;
    float learned = m_memory ? m_memory->GetAggression() : 0.0f;

    // Margin needed to commit. Defending is easier (self-preservation), hunting
    // needs a clear edge. AggroFactor shifts the margin: -40% config → ~2 extra
    // power needed to attack (the "half the aggression" the operator asked for).
    int margin = defending ? 0 : 2;
    margin += (int)std::lround(aggro * 4.0f);
    margin -= (int)std::lround(learned * 2.0f);   // winners need less margin

    bool wouldWin = (myPower - theirPower) >= margin;

    // Novices misjudge. A misread may make a novice PANIC and flee a winnable
    // fight — but never the reverse: a pilot that judged the fight as lost does
    // not suddenly attack. This keeps a hauler/frigate from suicide-charging a
    // supercarrier just because it's a bad judge of strength.
    float mistakeChance = 0.30f * (1.0f - skill);
    if (mistakeChance > 0.0f && MakeRandomFloat() < mistakeChance) {
        if (m_memory) m_memory->RecordPvpMistake();
        if (wouldWin)
            wouldWin = false;
    }
    return wouldWin;
}

bool PlayerBot::HunterWouldEngage(SystemEntity* target)
{
    // A hunter's decision to commit against a REAL player. Mirrors ShouldEngage
    // but tuned for player targets: real players are harder to predict, so a
    // hunter needs a clear edge and only veteran bots are confident. Used by the
    // NPCAI Idle scan so hunters occasionally prowl for players without the whole
    // crowd jumping them.
    if (target == nullptr || target->GetSelf().get() == nullptr || m_destiny == nullptr)
        return false;
    if (target->GetPilot() == nullptr)
        return false;   // not a real player — handled elsewhere

    Inv::TypeData tdata;
    sDataMgr.GetType(target->GetSelf()->typeID(), tdata);
    int enemyClass = GetShipClass(tdata.groupID);
    int myClass = GetShipClass(m_self->groupID());
    int myPower = myClass * 2 + (int)m_botSkill;
    int theirPower = enemyClass * 2 + 2;   // players are less predictable (+2)
    theirPower += CountEnemiesNearby(target) * 3;   // the target's friends add up
    myPower += CountAlliesNearby() * 2;             // my fleet helps me

    // Analytic threat, not a precise fit-check: a pilot never KNOWS the exact
    // fit on the other side. Capital ships (carriers/supers/titans) get a fleet
    // bonus — a cap can light a cyno and drop a fleet, and such ships rarely
    // travel alone. A ship that has fighters/drones out in space is a drone
    // carrier at least, which adds its fighter screen as real threat.
    using namespace EVEDB::invGroups;
    if (tdata.groupID == Carrier || tdata.groupID == Supercarrier || tdata.groupID == Titan)
        theirPower += 4;   // may cyno in friends / bring a fleet
    else if (tdata.groupID == Battleship || tdata.groupID == BlackOps || tdata.groupID == Marauder)
        theirPower += 1;   // heavy hull — assume it's fitted to fight

    // The ship's fighters/drones are already out — that's a real fighter screen
    // adding DPS and tackle, not just a hull class guess.
    if (target->GetDroneSE() != nullptr
        || (target->IsShipSE() && target->GetShipSE()->GetActiveFighterCount() > 0))
        theirPower += 3;

    float skill = m_memory ? m_memory->GetPvpSkill() : 0.0f;
    float aggro = (float)sConfig.playerBots.AggroFactor / 100.0f;
    float learned = m_memory ? m_memory->GetAggression() : 0.0f;
    int margin = 3 + (int)std::lround(aggro * 4.0f) - (int)std::lround(learned * 2.0f);

    bool wouldWin = (myPower - theirPower) >= margin;
    // Novices misjudge: they may chicken out of a winnable fight, but they never
    // misjudge INTO an attack on a stronger opponent (self-preservation wins).
    float mistakeChance = 0.30f * (1.0f - skill);
    if (mistakeChance > 0.0f && MakeRandomFloat() < mistakeChance) {
        if (m_memory) m_memory->RecordPvpMistake();
        if (wouldWin)
            wouldWin = false;
    }
    return wouldWin;
}

int PlayerBot::GetFaction() const
{
    // The bot's starter corp (from its race's school) tells its faction. Real EVE
    // lore: the four empires have been at war forever — a Caldari and an Amarr,
    // a Gallente and a Minmatar, etc. are natural enemies.
    switch (m_botCorpID) {
        // Amarr starter corps
        case 1000166: case 1000165: case 1000077:  return 4;
        // Minmatar starter corps
        case 1000170: case 1000171: case 1000172:  return 2;
        // Caldari starter corps
        case 1000167: case 1000045: case 1000044:  return 1;
        // Gallente starter corps
        case 1000168: case 1000115: case 1000169:  return 8;
        default:  return 0;   // neutral / unaffiliated (Society of Conscious Thought, etc.)
    }
}

bool PlayerBot::IsFactionEnemy(const PlayerBot* other) const
{
    if (other == nullptr || other == this)
        return false;
    int myFac = GetFaction();
    int theirFac = other->GetFaction();
    if (myFac == 0 || theirFac == 0)
        return false;   // neutrals don't pick fights on principle
    return myFac != theirFac;
}

void PlayerBot::UpdateBotStandings(const PlayerBot* other, bool otherLost)
{
    // After a fight, both pilots' opinions shift: the winner resents the loser
    // less, the loser hates the winner (and its corp). Persisted in repStandings
    // like every other standing — so over time whole corps drift apart and the
    // aggression/faction can be read from standings, not just flags.
    if (other == nullptr || other == this)
        return;

    // Personal grudge: loser hates winner (big hit), winner is wary of loser
    // (small hit, it attacked us). Neutral 0 if never fought.
    float meToOther = StandingDB::GetStanding(m_botCharID, other->GetBotCharID());
    float otherToMe = StandingDB::GetStanding(other->GetBotCharID(), m_botCharID);

    // Corp-level too: the loser's corp loses standing with the winner's corp.
    float corpToCorp = StandingDB::GetStanding(m_botCorpID, other->GetBotCorpID());

    if (otherLost) {
        // I won — the loser resents me; I'm slightly wary of it.
        StandingDB::UpdateStanding(m_botCharID, other->GetBotCharID(), -0.4f);      // me -> loser: wary
        StandingDB::UpdateStanding(other->GetBotCharID(), m_botCharID, -1.2f);      // loser -> me: hates
        StandingDB::UpdateStanding(m_botCorpID, other->GetBotCorpID(), -0.3f);      // corp -> corp
        StandingDB::UpdateStanding(other->GetBotCorpID(), m_botCorpID, -0.6f);
    } else {
        // I lost (or fled) — I resent it a lot; it's confident toward me.
        StandingDB::UpdateStanding(m_botCharID, other->GetBotCharID(), -1.0f);
        StandingDB::UpdateStanding(other->GetBotCharID(), m_botCharID, -0.3f);
        StandingDB::UpdateStanding(m_botCorpID, other->GetBotCorpID(), -0.5f);
        StandingDB::UpdateStanding(other->GetBotCorpID(), m_botCorpID, -0.3f);
    }

    _log(BOT__TRACE, "PlayerBot %s(%u) standings: to %s=%+.2f, from=%+.2f, corp %u vs %u=%+.2f.",
         m_botName.c_str(), m_botCharID, other->GetBotName().c_str(),
         StandingDB::GetStanding(m_botCharID, other->GetBotCharID()) - meToOther,
         StandingDB::GetStanding(other->GetBotCharID(), m_botCharID) - otherToMe,
         m_botCorpID, other->GetBotCorpID(),
         StandingDB::GetStanding(m_botCorpID, other->GetBotCorpID()) - corpToCorp);
}

uint8 PlayerBot::GetDroneCapacity() const
{
    // Drone bay capacity from the hull's REAL DroneCapacity attribute (283, m3).
    // Most ships carry some drones (Vexor 125m3, Dominix 200m3, Raven 50m3...).
    // A combat drone is ~25 m3, and EVE caps active drones at 5 (AttrMaxActiveDrones),
    // so the fieldable count = bay/25, clamped 1..5. 0 = no drone bay.
    if (m_self->HasAttribute(AttrDroneCapacity)) {
        float cap = m_self->GetAttribute(AttrDroneCapacity).get_float();
        int n = (int)(cap / 25.0f);
        if (n < 1) n = 1;
        if (n > 5) n = 5;
        return (uint8)n;
    }
    return 0;
}

void PlayerBot::SpawnDrones(uint8 count)
{
    // Create combat drones around the bot. They are real DroneSE entities so the
    // client renders them; the bot commands them directly (Orbit/Follow) without
    // DroneAI (which requires a ShipSE owner).
    if (SystemMgr() == nullptr || m_destiny == nullptr)
        return;
    uint8 cap = GetDroneCapacity();
    if (cap == 0)
        return;
    if (count == 0 || count > cap)
        count = cap;
    // Don't stack more than the bay allows.
    if ((uint8)m_drones.size() >= cap)
        return;

    static const uint32 combatDroneTypes[] = { 2454, 2486, 2183, 2203 };   // Hobgoblin/Warrior/Hammerhead/Acolyte
    DBQueryResult res;
    for (uint8 i = 0; i < count && (uint8)m_drones.size() < cap; ++i) {
        uint16 dType = combatDroneTypes[MakeRandomInt(0, 3)];
        // Resolve the drone item (must exist in this server's DB).
        Inv::TypeData tdata;
        sDataMgr.GetType(dType, tdata);
        if (tdata.id != dType)
            continue;

        // Spawn a drone item at the bot's position, then a DroneSE around it.
        ItemData idata(dType, m_botCharID, SystemMgr()->GetID(), flagNone, "Drone", GetPosition());
        InventoryItemRef dRef = sItemFactory.SpawnItem(idata);
        if (dRef.get() == nullptr)
            continue;
        dRef->ChangeSingleton(true);
        dRef->SetPosition(GetPosition());

        FactionData data = FactionData();
            data.corporationID = m_botCorpID;
            data.ownerID = m_botCharID;
            data.allianceID = m_botAllianceID;
        DroneSE* drone = new DroneSE(dRef, SystemMgr()->GetServiceMgr(), SystemMgr(), data);
        if (drone == nullptr) { dRef->Delete(); continue; }
        drone->Enable();                       // keep it alive (not pending removal)
        drone->SetDisplayOwner(m_botCharID, m_botCharID, GetID());
        // Register in the system/bubble so clients see it.
        SystemMgr()->AddEntity(drone);
        // Broadcast OnDroneStateChange so a player's drone window (stateByDroneID)
        // picks the drone up. Without it the drone renders as a ball but never
        // appears in the drone window ("I've never seen a chelobot's drone").
        drone->StateChange();
        // Orbit the bot while idle.
        if (drone->DestinyMgr() != nullptr) {
            drone->DestinyMgr()->SetPosition(GetPosition());
            drone->DestinyMgr()->SetMaxVelocity(dRef->GetAttribute(AttrMaxVelocity).get_float());
            drone->DestinyMgr()->SetSpeedFraction(0.6f);
            drone->DestinyMgr()->Orbit(this, 600.0);
        }
        m_drones.push_back(drone);
        _log(BOT__TRACE, "PlayerBot %s(%u): launched drone %u (%u) — bay %u/%u.",
             m_botName.c_str(), m_botCharID, drone->GetID(), dType, (uint32)m_drones.size(), cap);
    }
}

void PlayerBot::DroneEngageTarget(DroneSE* drone, SystemEntity* target)
{
    // Direct a drone to attack a target: fly to it, orbit at weapon range, and
    // apply damage each attack cycle (same math as DroneAI::CombatAttack but
    // without a ShipSE owner).
    if (drone == nullptr || target == nullptr || drone->DestinyMgr() == nullptr)
        return;
    if (drone->IsDead() || target->IsDead())
        return;
    if (target->DestinyMgr() == nullptr)
        return;   // target is a structure/decor — no ship to damage

    double dist = drone->GetPosition().distance(target->GetPosition());
    if (dist > 20000) {
        // far: chase it
        drone->DestinyMgr()->SetMaxVelocity(drone->GetSelf()->GetAttribute(AttrMaxVelocity).get_float());
        drone->DestinyMgr()->Follow(target, 1000.0);
        return;
    }
    // in weapon range: orbit and fire
    if (!drone->DestinyMgr()->IsOrbiting())
        drone->DestinyMgr()->Orbit(target, 800.0);

    if (!m_droneTimer.Enabled())
        m_droneTimer.Start(2000);
    if (!m_droneTimer.Check())
        return;

    // Drone damage — same formula as DroneAI::CombatAttack (owner-less skills).
    Damage d(drone,
             drone->GetSelf(),
             drone->GetKinetic(),
             drone->GetThermal(),
             drone->GetEM(),
             drone->GetExplosive(),
             1.0f,   // to-hit — drones don't miss in our simplification
             EVEEffectID::targetAttack);
    float dmgMult = drone->GetSelf()->HasAttribute(AttrDamageMultiplier)
        ? drone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() : 1.0f;
    d *= dmgMult;
    target->ApplyDamage(d);

    // Visible weapon effect from the drone to the target.
    if (drone->SysBubble() != nullptr)
        drone->DestinyMgr()->SendSpecialEffect(drone->GetSelf()->itemID(), drone->GetSelf()->itemID(),
                                               drone->GetSelf()->typeID(), target->GetID(), 0,
                                               "effects.StandardWeapon", 1, 1, 1, 2000, 0, 0);
}

void PlayerBot::ManageDrones()
{
    // Called each Process tick. Drones orbit the bot when idle; during a fight
    // (own target, or an ally's target for assist) they engage.
    if (m_drones.empty())
        return;
    if (SystemMgr() == nullptr || m_destiny == nullptr)
        return;

    // Pick the target: my own current target, else an ally's (assist).
    SystemEntity* target = nullptr;
    if (GetAIMgr()->IsFighting() && m_targMgr != nullptr)
        target = m_targMgr->GetFirstTarget(true);

    // Assist: if an ally bot is fighting, send my drones at its target too.
    if (target == nullptr) {
        for (auto& [id, se] : SystemMgr()->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* ally = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (ally == nullptr || ally == this)
                continue;
            if (ally->GetBotCorpID() != m_botCorpID && ally->GetBotAllianceID() != m_botAllianceID)
                continue;
            if (!ally->GetAIMgr()->IsFighting() || ally->m_targMgr == nullptr)
                continue;
            SystemEntity* allyTarget = ally->m_targMgr->GetFirstTarget(true);
            if (allyTarget != nullptr) { target = allyTarget; break; }
        }
    }

    // Drone wants to be near the bot when idle. Leash: drones farther than 30km
    // drift back to the bot.
    for (auto it = m_drones.begin(); it != m_drones.end(); ) {
        DroneSE* drone = *it;
        if (drone == nullptr) {
            it = m_drones.erase(it);
            continue;
        }
        // Dead drones: SystemEntity::Killed already removed the entity and item,
        // so we must NOT call RemoveEntity/GetSelf()->Delete() on them — just
        // drop the wrapper (and free it without touching the freed item).
        if (drone->IsDead()) {
            drone->RemoveDead();
            it = m_drones.erase(it);
            continue;
        }
        if (drone->DestinyMgr() == nullptr) {
            it = m_drones.erase(it);
            continue;
        }
        double distToBot = drone->GetPosition().distance(GetPosition());
        if (distToBot > 30000 || drone->IsPendingRemoval() || drone->IsEnabled() == false) {
            // scoop / cleanup
            drone->ScoopAndDelete();
            it = m_drones.erase(it);
            continue;
        }

        if (target != nullptr && distToBot < 25000) {
            DroneEngageTarget(drone, target);   // attack (own or assist target)
        } else if (!drone->DestinyMgr()->IsOrbiting()) {
            // idle: orbit the bot
            drone->DestinyMgr()->SetMaxVelocity(drone->GetSelf()->GetAttribute(AttrMaxVelocity).get_float());
            drone->DestinyMgr()->SetSpeedFraction(0.6f);
            drone->DestinyMgr()->SetPosition(GetPosition(), true);
            drone->DestinyMgr()->Orbit(this, 600.0);
        }
        ++it;
    }
}

void PlayerBot::RecallDrones()
{
    // Scoop all drones (flee / dock / travel): return them to the bot and remove.
    if (m_drones.empty())
        return;
    for (DroneSE* drone : m_drones) {
        if (drone == nullptr)
            continue;
        if (drone->IsDead()) {
            drone->RemoveDead();   // killed in combat — item already freed
            continue;
        }
        drone->ScoopAndDelete();   // alive — remove entity, delete item, free wrapper
    }
    m_drones.clear();
}

void PlayerBot::BroadcastAggression(uint32 victimCharID)
{
    // Players see an attacker's blinking aggression icon via the OnAggressionChange
    // notification: aggressors[attackerCharID] = {victimID: endTime}. A bot that
    // attacks broadcasts itself as the aggressor so its icon flashes like any
    // player's. endTime is FILETIME (100ns ticks) in the future.
    if (m_destiny == nullptr || SysBubble() == nullptr)
        return;
    int64 now = GetFileTimeNow();
    int64 end = now + m_aggressionTimer.GetRemainingTime() * 10000LL;
    if (victimCharID == 0)
        return;

    PyDict* timers = new PyDict();
        timers->SetItem(new PyInt(victimCharID), new PyLong(end));
    PyDict* aggressors = new PyDict();
        aggressors->SetItem(new PyInt(m_botCharID), timers);
    PyTuple* payload = new PyTuple(2);
        payload->SetItem(0, new PyInt(SystemMgr()->GetID()));
        payload->SetItem(1, aggressors);
    SysBubble()->BubblecastSendNotification("OnAggressionChange", "solarsystemid", &payload, true);
}

void PlayerBot::ApplyCombatStyle()
{
    if (GetAIMgr() == nullptr)
        return;
    uint32 optimal = GetAIMgr()->GetOptimalRange();
    if (optimal == 0) optimal = 15000;
    switch (m_combatStyle) {
        case CombatStyle::Kite:
            // Keep range: orbit just outside the enemy's reach, inside ours.
            GetAIMgr()->SetOrbitRange(optimal + optimal / 2);
            break;
        case CombatStyle::Brawler:
            // Close in: orbit tight around the target.
            GetAIMgr()->SetOrbitRange(optimal > 8000 ? 8000 : (optimal / 2));
            break;
        default:
            // Balanced: orbit at weapon optimal (the default).
            GetAIMgr()->SetOrbitRange(0);
            break;
    }
}

void PlayerBot::DecideNextAction()
{
    // Hook for BotMgr. Placeholder for the state machine that will be wired
    // to actual travel/combat/mining behaviour.
    _log(BOT__TRACE, "PlayerBot %s(%u): activity = %u, system = %u",
         m_botName.c_str(), m_botCharID, (uint8)m_activity, SystemMgr()->GetID());
}

void PlayerBot::AddCargo(uint16 typeID, uint32 qty)
{
    if (typeID == 0 || qty == 0)
        return;
    m_cargo[typeID] += qty;
}

bool PlayerBot::HasCargo() const
{
    for (const auto& entry : m_cargo)
        if (entry.second > 0)
            return true;
    return false;
}

float PlayerBot::GetCargoVolume() const
{
    float vol = 0.0f;
    for (const auto& entry : m_cargo) {
        if (entry.second == 0)
            continue;
        const ItemType* t = sItemFactory.GetType(entry.first);
        float unit = t != nullptr ? t->volume() : 1.0f;
        if (unit < 0.01f) unit = 1.0f;
        vol += unit * entry.second;
    }
    return vol;
}

// After a rat/hack kill the bot tractor-beams in the wrecks IT created (the
// NPC death already filled them with real loot via DropLoot), scoops that loot
// into its hold, salvages the hull (a few salvage materials), and removes the
// wreck. Only wrecks owned by this bot's own hull are touched — other pilots'
// wrecks (players, other bots) are left alone.
uint32 PlayerBot::SalvageMyWrecks()
{
    if (SystemMgr() == nullptr || m_destiny == nullptr)
        return 0;

    // Which hull are we? Wrecks we caused have ownerID == our ship itemID.
    uint32 myHull = m_self->itemID();
    uint32 salvaged = 0;

    std::vector<SystemEntity*> wrecks;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || !se->IsWreckSE())
            continue;
        wrecks.push_back(se);
    }

    for (SystemEntity* wse : wrecks) {
        InventoryItemRef wreckRef = wse->GetSelf();
        if (wreckRef.get() == nullptr)
            continue;
        if (wreckRef->ownerID() != myHull)
            continue;   // not ours — leave for its owner
        uint32 wreckID = wreckRef->itemID();

        // Scoop the wreck's contents into our hold (real loot from DropLoot).
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT typeID, SUM(quantity) FROM entity WHERE locationID = %u AND flag = %u GROUP BY typeID",
            wreckID, (uint32)flagNone))
        {
            DBResultRow row;
            while (res.GetRow(row)) {
                uint16 typeID = (uint16)row.GetUInt(0);
                uint32 qty = row.GetUInt(1);
                if (typeID != 0 && qty > 0)
                    AddCargo(typeID, qty);
            }
        }
        // Delete the loot item rows (they move to junkyard / vanish).
        DBQueryResult cres;
        if (sDatabase.RunQuery(cres, "SELECT itemID FROM entity WHERE locationID = %u AND flag = %u", wreckID, (uint32)flagNone)) {
            DBResultRow cRow;
            while (cres.GetRow(cRow)) {
                InventoryItemRef itm = sItemFactory.GetItemRef(cRow.GetUInt(0));
                if (itm.get() != nullptr)
                    itm->Delete();
            }
        }

        // Salvage the hull: a couple of random salvage materials.
        static const uint16 salvageTypes[] = { 25588, 25589, 25590, 25591, 25593, 25594, 25599, 25605 };
        uint32 nSalvage = MakeRandomInt(0, 3);
        for (uint32 s = 0; s < nSalvage; ++s)
            AddCargo(salvageTypes[MakeRandomInt(0, 7)], MakeRandomInt(1, 3));

        // Remove the wreck entity.
        wse->Delete();
        ++salvaged;
    }

    if (salvaged > 0)
        _log(BOT__TRACE, "PlayerBot %s(%u): salvaged %u of its wrecks (hold %.0f m3).",
             m_botName.c_str(), m_botCharID, salvaged, GetCargoVolume());
    return salvaged;
}

// Real physical deposit: everything the bot is carrying is spawned as actual
// entity items in the station hangar, owned by the bot. (Same mechanism a sell
// order fill uses — SpawnItem in limbo, then Donate into the station hangar.)
// After this the station physically holds minerals/loot that a trader bot can
// pick up and pack into a courier contract. Returns units deposited.
double PlayerBot::DepositCargoAtStation(uint32 stationID)
{
    if (stationID == 0 || !HasCargo())
        return 0.0;

    double units = 0.0;
    for (auto& entry : m_cargo) {
        uint16 typeID = entry.first;
        uint32 qty = entry.second;
        if (qty == 0)
            continue;
        // Spawn in limbo then hand to the bot's hangar at the station.
        ItemData idata(typeID, ownerStation, locTemp, flagNone, qty);
        InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
        if (iRef.get() == nullptr) {
            _log(BOT__ERROR, "PlayerBot %s(%u): failed to deposit %u x type %u at station %u.",
                 m_botName.c_str(), m_botCharID, qty, typeID, stationID);
            continue;
        }
        iRef->Donate(m_botCharID, stationID, flagHangar, false);
        units += qty;
        _log(BOT__TRACE, "PlayerBot %s(%u): deposited %u x type %u in hangar at station %u.",
             m_botName.c_str(), m_botCharID, qty, typeID, stationID);
    }
    m_cargo.clear();
    return units;
}

// A bot levels up when its accumulated practice (profession runs / PvP) crosses
// the threshold for the next tier. On level-up its real skill items are trained
// to the new level (CharacterDB::TrainBotToSkillLevel) so a veteran genuinely
// out-performs a rookie — and the tier is persisted so it survives respawns.
bool PlayerBot::LevelUpFromPractice()
{
    if (m_memory == nullptr)
        return false;
    uint8 cur = m_memory->GetSkillLevel();
    if (!m_memory->HasSkillLevel()) {
        // Legacy bot (no stored tier). Seed it from the config roll used at
        // character creation, then persist. Further gains level from practice.
        uint8 seed = sConfig.playerBots.MinSkillLevel + (uint8)MakeRandomInt(
            0, sConfig.playerBots.MaxSkillLevel - sConfig.playerBots.MinSkillLevel);
        m_memory->SetSkillLevel(seed);
        m_botSkill = seed;
        CharacterDB::TrainBotToSkillLevel(m_botCharID, seed);
        m_memory->Save();
        return false;
    }
    if (cur >= 5)
        return false;

    uint32 need = BotMemory::PracticeForNextLevel(cur);
    uint32 have = m_memory->GetPractice();
    if (have < need)
        return false;

    uint8 next = cur + 1;
    m_memory->SetSkillLevel(next);
    m_botSkill = next;
    CharacterDB::TrainBotToSkillLevel(m_botCharID, next);
    m_memory->Save();
    _log(BOT__MESSAGE, "PlayerBot %s(%u): levelled up to skill tier %u (practice %u >= %u).",
         m_botName.c_str(), m_botCharID, next, have, need);
    return true;
}

// Professional mining fleet boost (guide model): a barge mining near a friendly
// fleet boss (Orca 28606 / Rorqual 28352 — Industrial Command Ship / Capital
// Industrial) of the same corp mines faster, as if the boss ran mining foreman
// links. Returns a yield multiplier: 1.0 alone, up to ~1.3 with a boss close.
float PlayerBot::GetFleetMiningBoost()
{
    if (SystemMgr() == nullptr)
        return 1.0f;
    float boost = 1.0f;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (other == nullptr || other == this)
            continue;
        if (other->GetBotCorpID() != m_botCorpID)
            continue;
        if (!other->IsFleetBoss())
            continue;
        double d = GetPosition().distance(other->GetPosition());
        if (d < 80000.0) {                 // within ~80 km of the boss
            boost = 1.3f;
            break;
        }
    }
    return boost;
}

void PlayerBot::DoProfessionActivity()
{
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;

    // Self-learning: practiced bots act more often / more efficiently.
    float practice = m_memory ? m_memory->GetActivitySkill() : 0.0f;
    float chanceBoost = 30.0f + practice * 40.0f;   // 30%..70% action chance

    // ---- No-station behaviour ----
    // Station-hugging professions (miner refining, trader at the market, courier,
    // hacker hauling loot) can't do their normal job in a system with no station.
    // A real pilot doesn't stand at the gate forever — they drift around and,
    // if they've been around a while, head toward the trade hub to work.
    if (!HasStationInSystem()) {
        bool stationProf = (m_profession == BotProfession::Trader
                         || m_profession == BotProfession::Courier
                         || m_profession == BotProfession::Miner
                         || m_profession == BotProfession::Hacker);
        if (stationProf) {
            // Experienced bots know their way to the market; novices wander first.
            bool experienced = m_memory && (m_memory->GetKills() + m_memory->GetTradeRuns() + m_memory->GetHackRuns() > 0);
            if (experienced) {
                if (MakeRandomInt(0, 99) < 60) {
                    HeadTowardHub(sBotMgr.GetTradeHubSystem());
                    return;
                }
            } else {
                PatrolForIdle();
                if (MakeRandomInt(0, 99) < 30)
                    HeadTowardHub(sBotMgr.GetTradeHubSystem());
                return;
            }
        }
        // Hunters/ratters/explorers work fine without a station — fall through.
    }

    switch (m_profession) {
        case BotProfession::Hunter: {
            // PvP pirates hunt; guards (fighter/support role) stick with their
            // corp's industrials instead of roaming.
            bool isGuard = (m_role == BotRole::Fighter || m_role == BotRole::Support);
            if (isGuard && MakeRandomInt(0, 99) < 60) {
                // Escort: stay near the closest industrial corpmate (miner/hauler).
                PlayerBot* ward = nullptr;
                double bestD = 100000;
                for (auto& [id, se] : SystemMgr()->GetEntities()) {
                    if (se == nullptr || se->GetNPCSE() == nullptr)
                        continue;
                    PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
                    if (other == nullptr || other == this)
                        continue;
                    if (other->GetBotCorpID() != m_botCorpID)
                        continue;
                    auto prof = other->GetProfession();
                    if (prof != BotProfession::Miner && prof != BotProfession::Courier && prof != BotProfession::Trader)
                        continue;
                    double d = GetPosition().distance(other->GetPosition());
                    if (d < bestD) { bestD = d; ward = other; }
                }
                if (ward != nullptr && bestD > 10000 && !m_destiny->IsWarping()) {
                    m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
                    m_destiny->WarpTo(ward->GetPosition(), 1500);
                    _log(BOT__TRACE, "PlayerBot %s(%u): guard escorting %s(%u).",
                         m_botName.c_str(), m_botCharID, ward->GetBotName().c_str(), ward->GetBotCharID());
                    return;
                }
            }
            // Otherwise hunt for legal PvP targets in lowsec/nullsec.
            HuntForTarget();
            // PvP war corps claim unowned nullsec (skirmish).
            if (SystemMgr()->GetSystemSecurityRating() < 0.0f)
                ClaimSystem();
        } break;

        case BotProfession::RatHunter: {
            // Peaceful PvE: only engage NPC red crosses (ratting), never players.
            RatForTarget();
            // When the loot hold fills up, head to the station to deposit it
            // (real salvage + faction loot from the wrecks the bot made).
            if (GetCargoVolume() >= 10000.0f && HasStationInSystem()) {
                RequestDock();
                _log(BOT__TRACE, "PlayerBot %s(%u): loot hold full — docking to deposit.",
                     m_botName.c_str(), m_botCharID);
            }
        } break;

        case BotProfession::Miner: {
            // Peaceful miner: fly/warp to a belt asteroid and sit mining. In a fleet
            // (same corp) miners cooperate at one belt; guard fighters protect them.
            // Real physical ore: while sitting at the rock the bot fills its hold
            // (m_cargo) with the asteroid's ore type, then docks to deposit it.
            BeltMgr* beltMgr = SystemMgr()->GetBeltMgr();
            if (beltMgr != nullptr) {
                AsteroidSE* roid = beltMgr->GetAnyAsteroid();
                if (roid != nullptr && roid->DestinyMgr() != nullptr && !m_destiny->IsWarping()) {
                    double dist = GetPosition().distance(roid->GetPosition());
                    if (dist > 4000.0) {
                        // Not at the rock yet — warp to it (visible approach).
                        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed() / 2);
                        m_destiny->WarpTo(roid->GetPosition(), 1000);
                        _log(BOT__TRACE, "PlayerBot %s(%u): mining — warping to asteroid %u.",
                             m_botName.c_str(), m_botCharID, roid->GetID());
                    } else {
                        // Sitting at the rock: mine a strip-miner cycle into the hold.
                        // Ore type = the asteroid's own ore type. Cycle volume scales
                        // with the pilot's skill tier (a trained veteran mines faster).
                        InventoryItemRef roidRef = roid->GetSelf();
                        if (roidRef.get() != nullptr) {
                            uint16 oreType = roidRef->typeID();
                            float oreUnitVol = roidRef->GetAttribute(AttrVolume).get_float();
                            if (oreUnitVol < 0.1f) oreUnitVol = 1.0f;
                            float cycleVol = 100.0f + m_botSkill * 60.0f;   // m3 per cycle (skill-scaled)
                            cycleVol *= GetFleetMiningBoost();              // fleet boss yields more
                            uint32 units = (uint32)(cycleVol / oreUnitVol);
                            if (units < 1) units = 1;
                            // Don't overfill a modest hold (barge-ish).
                            if (GetCargoVolume() + cycleVol > 15000.0f)
                                units = 0;
                            if (units > 0) {
                                AddCargo(oreType, units);
                                _log(BOT__TRACE, "PlayerBot %s(%u): mined %u x ore %u (%.0f m3) into hold.",
                                     m_botName.c_str(), m_botCharID, units, oreType, cycleVol);
                            }
                        }
                    }
                }
            }
            // Cooperative mining: ask corpmates (guards) to cover this miner.
            RequestFleetProtection();
            // End of mining run: head to the station to deposit the ore. Experienced
            // miners haul more ore per trip (self-learning), so longer runs.
            float runLen = 2.0f + practice * 6.0f;   // 2..8 trips between docks
            if (m_mineTrips >= runLen) {
                m_mineTrips = 0;
                RequestDock();
                _log(BOT__TRACE, "PlayerBot %s(%u): ore hold full — docking to deposit.",
                     m_botName.c_str(), m_botCharID);
            } else {
                ++m_mineTrips;
            }
        } break;

        case BotProfession::Trader:
        case BotProfession::Courier: {
            // Peaceful trader/courier: mostly WORK the station — sit at the
            // market, occasionally head out. Experienced traders route more often.
            if (m_profession == BotProfession::Trader) {
                // Traders live at the station; short excursions between orders.
                if (MakeRandomInt(0, 99) < (int)(70 + practice * 20))
                    RequestDock();   // sitting at the market, buying/selling
                else
                    ClearDockRequest();
            }
            for (auto& [id, se] : SystemMgr()->GetStaticEntities()) {
                if (se != nullptr && (se->GetStationSE() != nullptr || se->GetGateSE() != nullptr)) {
                    if (!m_destiny->IsWarping() && MakeRandomInt(0, 99) < (int)chanceBoost) {
                        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
                        m_destiny->WarpTo(se->GetPosition(), 2000);
                        _log(BOT__TRACE, "PlayerBot %s(%u): %s — moving to station/gate.",
                             m_botName.c_str(), m_botCharID,
                             m_profession == BotProfession::Trader ? "trading" : "courier");
                    }
                    break;
                }
            }
        } break;

        case BotProfession::Hacker: {
            // Peaceful hacker: run data/relic sites — warp to a Magnetometric
            // (data) or Radar (relic) signature, "crack" containers there and
            // pull real datacores/decryptors into the hold, then dock to
            // deposit them (BotMgr turns the hangar stock into ISK at the hub).
            HackForSites();
        } break;

        case BotProfession::Explorer: {
            // Wormhole/scan explorer: deploys probes, finds signatures and
            // wormholes, works them, hauls loot to the hub. Explores null/w-space.
            ScanForSites();
        } break;
    }
}

void PlayerBot::ScanForSites()
{
    // Explorer: scan for signatures / wormholes, work them, haul loot to the hub.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (m_destiny->IsWarping())
        return;

    // Find a wormhole in this system to work.
    SystemEntity* target = nullptr;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetWormholeSE() == nullptr)
            continue;
        target = se;
        break;
    }
    // Else an anomaly/site.
    if (target == nullptr) {
        for (auto& [id, se] : SystemMgr()->GetEntities()) {
            if (se == nullptr || se->GetAnomalySE() == nullptr)
                continue;
            target = se;
            break;
        }
    }

    if (target != nullptr) {
        double d = GetPosition().distance(target->GetPosition());
        if (d > 5000 && !m_destiny->IsWarping()) {
            m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
            m_destiny->WarpTo(target->GetPosition(), 1500);
            _log(BOT__TRACE, "PlayerBot %s(%u): explorer — scanning/warping to site %s.",
                 m_botName.c_str(), m_botCharID, target->GetName());
            return;
        }
        // Standing at the site: "scanning" earns loot — an explorer running a
        // relic/data site pulls real datacores/decryptors into the hold, then
        // docks to deposit them (same physical-goods chain as miners/ratters).
        AddHackLoot();
        // After several scans, haul the loot back to sell. With a local station
        // the explorer docks and deposits; in a station-less system (wormhole /
        // null) it keeps the dock request and moves on — the moment it reaches a
        // system with a station, BotMgr docks it and the hold is banked there.
        if (m_mineTrips++ > 5) {
            m_mineTrips = 0;
            if (HasStationInSystem())
                RequestDock();   // go home, sell the loot
            else {
                RequestDock();
                MarkForTravel(); // keep hunting a station-equipped system
            }
        }
        return;
    }
    // No sites here — move on (explore another system).
    if (MakeRandomInt(0, 99) < 40)
        MarkForTravel();
}

// Data/relic site runner (hacker). Real physical loot: warp to a Magnetometric
// (data) or Radar (relic) cosmic signature the system's AnomalyMgr knows about,
// crack its containers for datacores/decryptors/salvage into the hold, then dock
// to deposit them. Without a data/relic site in this system the hacker moves on.
void PlayerBot::HackForSites()
{
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (m_destiny->IsWarping())
        return;

    // Hackers work data/relic sites — those need probing in the client, but a
    // veteran hacker "already scanned it down" (it's how its AI knows the site).
    GPoint sitePos;
    bool found = false;
    AnomalyMgr* anom = SystemMgr()->GetAnomMgr();
    if (anom != nullptr) {
        std::vector<CosmicSignature> sigs;
        anom->GetSignatureList(sigs);
        // Prefer relic (4), else data (3); pick a random one if several.
        for (int pass = 0; pass < 2 && !found; ++pass) {
            int8 wantType = (pass == 0) ? Dungeon::Type::Radar : Dungeon::Type::Magnetometric;
            std::vector<CosmicSignature> matching;
            for (const auto& s : sigs)
                if (s.dungeonType == wantType && s.sigItemID != 0)
                    matching.push_back(s);
            if (!matching.empty()) {
                sitePos = matching[MakeRandomInt(0, (int64)matching.size() - 1)].position;
                found = true;
            }
        }
    }
    if (!found) {
        // No data/relic site in this system — run the next system over.
        if (MakeRandomInt(0, 99) < 40)
            MarkForTravel();
        return;
    }

    double dist = GetPosition().distance(sitePos);
    if (dist > 25000 && !m_destiny->IsWarping()) {
        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
        m_destiny->WarpTo(sitePos, 2000);
        _log(BOT__TRACE, "PlayerBot %s(%u): hacker — warping to data/relic site.",
             m_botName.c_str(), m_botCharID);
        return;
    }

    // At the site: crack a container on this pass, pull real loot into the hold.
    AddHackLoot();

    // Haul it back when the run is done (self-learning lengthens runs).
    float practice = m_memory ? m_memory->GetActivitySkill() : 0.0f;
    float runLen = 8.0f + practice * 12.0f;   // 8..20 container cracks between docks
    if (m_mineTrips++ >= runLen) {
        m_mineTrips = 0;
        if (HasStationInSystem())
            RequestDock();
        else
            MarkForTravel();
        _log(BOT__TRACE, "PlayerBot %s(%u): hacker run done — docking to deposit loot.",
             m_botName.c_str(), m_botCharID);
    }
}

// One "hack" tick at a worked site: accumulate real loot into m_cargo the way a
// data/relic runner pulls from hacked containers — datacores (group 333) and
// decryptors (the faction decryptor families) most often, salvage sometimes.
void PlayerBot::AddHackLoot()
{
    // A handful of common datacores a hacker would pull from relic containers.
    static const uint16 datacores[] = { 20171, 20172, 20410, 20411, 20412, 20413,
                                        20414, 20417, 20419, 20420, 20421, 25887 };
    const uint32 nDC = sizeof(datacores) / sizeof(datacores[0]);
    static const uint16 decryptors[] = { 23178, 23179, 23180, 23181, 23182,
                                         21579, 21580, 21581, 21582, 21583,
                                         23183, 23184, 23185, 23186, 23187,
                                         21573, 21574, 21575, 21576, 21577 };
    const uint32 nDec = sizeof(decryptors) / sizeof(decryptors[0]);
    static const uint16 salvage[] = { 25588, 25589, 25590, 25591, 25593, 25594, 25599, 25605 };
    const uint32 nSalv = sizeof(salvage) / sizeof(salvage[0]);

    // Skill-scaled: a practised hacker pulls more per container. Volume builds
    // so a full run is a courier-worthy hold (datacore/decryptor ~1 m3 each).
    uint32 pulls = 4 + m_botSkill * 2;        // 4..14 items per container crack
    for (uint32 i = 0; i < pulls; ++i) {
        uint32 r = MakeRandomInt(0, 99);
        if (r < 55)      // datacores — the staple
            AddCargo(datacores[MakeRandomInt(0, (int64)nDC - 1)], MakeRandomInt(1, 3));
        else if (r < 80) // decryptors — the prize (faction decryptor families)
            AddCargo(decryptors[MakeRandomInt(0, (int64)nDec - 1)], 1);
        else             // occasional salvage (scrap from the site)
            AddCargo(salvage[MakeRandomInt(0, (int64)nSalv - 1)], MakeRandomInt(1, 3));
    }

    if (m_memory && MakeRandomInt(0, 99) < 30) {
        m_memory->RecordHackRun();
        m_memory->Save();
    }

    _log(BOT__TRACE, "PlayerBot %s(%u): cracked a container (hold now %.0f m3).",
         m_botName.c_str(), m_botCharID, GetCargoVolume());
}

bool PlayerBot::HasStationInSystem()
{
    if (SystemMgr() == nullptr)
        return false;
    for (auto& [id, se] : SystemMgr()->GetStaticEntities())
        if (se != nullptr && se->GetStationSE() != nullptr)
            return true;
    return false;
}

void PlayerBot::PatrolForIdle()
{
    // A pilot stranded in a station-less system (wormhole, dead-end, pirate hole)
    // doesn't just sit at the gate. They drift around doing something plausible:
    // poke an anomaly, hover by a belt, orbit a gate. No spawns are required —
    // just visible movement so the system looks alive.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (m_destiny->IsWarping() || m_destiny->IsMoving())
        return;   // already doing something

    // Pick a random point of interest: a gate, a belt asteroid, or an anomaly.
    std::vector<GPoint> spots;
    for (auto& [id, se] : SystemMgr()->GetGates())
        if (se != nullptr)
            spots.push_back(se->GetPosition());
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr) continue;
        if (se->IsAsteroidSE() || se->IsAnomalySE() || se->GetWormholeSE() != nullptr)
            spots.push_back(se->GetPosition());
    }
    if (spots.empty())
        return;   // truly nothing — stay parked

    GPoint dest = spots[MakeRandomInt(0, (int64)spots.size() - 1)];
    // Land well clear of any collision sphere.
    m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed() * 0.6f);   // a cruising pilot, not a racer
    m_destiny->WarpTo(dest, 5000);
    _log(BOT__TRACE, "PlayerBot %s(%u): no station here — drifting to %.0f,%.0f,%.0f.",
         m_botName.c_str(), m_botCharID, dest.x, dest.y, dest.z);
}

void PlayerBot::HeadTowardHub(uint32 hubSystem)
{
    // Long-haul: this system has no station and isn't home — set course for the
    // trade hub. The travel machinery (ProcessTravel) picks it up and flies the
    // visible gate-crossing chain.
    if (hubSystem == 0 || hubSystem == SystemMgr()->GetID())
        return;
    SetTravelDestination(hubSystem);
    MarkForTravel(hubSystem);
    _log(BOT__TRACE, "PlayerBot %s(%u): no station here — heading to hub %u.",
         m_botName.c_str(), m_botCharID, hubSystem);
}

void PlayerBot::HuntForTarget()
{
    // Aggressive (hunter / PvP war corp): actively seek a legal target — but
    // like a real pilot, NOT at every gate. The bot scans first, judges the
    // value of the target and the risk (friends nearby, gate camp), and only
    // commits when it has a clear edge. Novices misjudge; veterans are precise.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    float sysSec = SystemMgr()->GetSystemSecurityRating();

    // Only hunt where PvP is viable (lowsec/nullsec mostly; highsec rarely).
    if (sysSec >= 0.5f && MakeRandomInt(0, 99) >= 5)
        return;

    // Hunt cooldown: a hunter doesn't camp the same spot forever — it sweeps,
    // then moves on. Between sweeps it wanders (looks busy, not static).
    if (m_huntCooldown.Enabled() && !m_huntCooldown.Check())
        return;
    m_huntCooldown.Start(MakeRandomInt(20000, 45000));   // ~20-45s between engages

    // A freshly-arrived hunter scouts first: scans the system for a few seconds
    // before committing to any fight (like a scout who warped in ahead). It still
    // logs who's here but holds fire until the scout window passes.
    bool scouting = m_scoutTimer.Enabled() && !m_scoutTimer.Check();
    if (!m_scoutTimer.Enabled())
        m_scoutTimer.Start(MakeRandomInt(15000, 30000));

    // Gate camping is a GROUP tactic, not a solo one: a lone pilot at a gate is
    // just bait (or a corpse). Only hunters who have allies nearby (a camp fleet)
    // will consider a target sitting at a gate — and even then only rarely and
    // when the victim has no friends of their own. Everyone else avoids the gate.
    int myAllies = CountAlliesNearby();
    bool canCampGate = (myAllies >= 1) && (MakeRandomInt(0, 99) < 20);

    // Find a target: enemy PlayerBots in this system. Score by distance but also
    // by "value" (a ratting miner or hauler is a prize; a big hostile fleet is a
    // trap). Targets at a gate are only for the rare group camp; solo hunters
    // avoid them (that's where camps and friends hide).
    SystemEntity* prey = nullptr;
    int bestScore = -1000;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* enemy = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (enemy == nullptr || enemy == this)
            continue;
        if (enemy->GetBotCorpID() == m_botCorpID || enemy->GetBotAllianceID() == m_botAllianceID)
            continue;   // ally
        // Who may be attacked — like real pilots, bots don't care about race.
        // Any bot of a DIFFERENT corp/alliance is a potential target; the decider
        // is where we are and how confident we are:
        //  - highsec: CONCORD protects everyone, so only fellow aggressive
        //    hunters (both flagged) risk a fight there.
        //  - lowsec/nullsec: open season on anyone of another corp/alliance.
        // A faction warrior (FW subclass stub) has FIXED enemies — bots of other
        // factions — and fights them everywhere, even in highsec (militia FW is
        // exempt from normal CONCORD rules). This is just hunting with a filter.
        bool factionEnemy = IsFactionEnemy(enemy);
        // A standing grudge overrides the usual rules: if THIS bot's corp has
        // clearly bad standings with the enemy's corp (past fights, wars), it's
        // a known enemy — treat it as fair game even in highsec (war targets).
        bool grudge = StandingDB::GetStanding(m_botCorpID, enemy->GetBotCorpID()) <= -1.0f
                   || StandingDB::GetStanding(m_botCorpID, enemy->GetBotCharID()) <= -1.0f;
        if (m_factionWarrior) {
            if (!factionEnemy)
                continue;   // FW fights only its faction's enemies
        } else if (grudge) {
            // known enemy (from standings) — hunt it anywhere
        } else if (sysSec >= 0.5f) {
            if (!enemy->IsAggressive())
                continue;   // highsec: only hunt aggressive targets (both flagged)
        } else if (!enemy->IsAggressive() && !m_memory) {
            continue;   // low/null: peaceful bots are still fair game for hunters,
                        // but a novice hunter passes on them until it learns to fight
        }
        if (enemy->IsNearGate(60000.0)) {
            if (!canCampGate)
                continue;   // solo: avoid the gate entirely (ambush risk)
            // group camp: still only if the victim looks alone and weaker
            if (CountEnemiesNearby(enemy) > 0)
                continue;   // victim has friends — the camp would turn into a brawl
        }
        // Combat-probe scan: a hunter with probes "finds" targets across the
        // whole system (battlefield/asteroid/anomaly), not just its own bubble.
        double d = GetPosition().distance(enemy->GetPosition());
        if (d > 250000)
            continue;   // beyond probe range
        int score = (int)(250000 - d) / 1000;
        // Value: miners/haulers/traders are good loot and weak; hunters are not.
        auto prof = enemy->GetProfession();
        if (prof == BotProfession::Miner || prof == BotProfession::Courier
            || prof == BotProfession::Trader || prof == BotProfession::Hacker)
            score += 30;
        else if (prof == BotProfession::Hunter)
            score -= 50;   // fellow hunters fight back — only if we're confident
        // A faction warrior prizes its fixed enemies a little more (war worth).
        if (m_factionWarrior && factionEnemy)
            score += 25;
        // Risk: friends near the target lower the score hard (bait check).
        score -= CountEnemiesNearby(enemy) * 40;
        if (score > bestScore) { bestScore = score; prey = enemy; }
    }

    // While scouting (fresh arrival), the bot only reports, never commits to a
    // fight — a scout doesn't engage, it feeds intel back to the fleet.
    if (scouting)
        return;

    // Evaluate the fight like a pilot: my power vs theirs, adjusted by how many
    // of their friends are around. If favoured, engage; else leave it alone.
    if (prey != nullptr && bestScore > -20) {
        PlayerBot* enemyBot = (PlayerBot*)prey;
        int myClass = GetShipClass(m_self->groupID());
        int enemyClass = GetShipClass(enemyBot->GetSelf()->groupID());
        int myPower = myClass * 2 + (int)m_botSkill;
        int theirPower = enemyClass * 2 + (int)enemyBot->GetBotSkillLevel();
        theirPower += CountEnemiesNearby(enemyBot) * 3;   // friends add to their strength
        myPower += CountAlliesNearby() * 2;               // my fleet helps me
        // Bots are bolder against each other than against players — an AI pilot is
        // a more predictable opponent, so the hunter commits a little easier.
        theirPower -= 2;
        // Analytic threat: capitals may cyno in a fleet, battleships are assumed
        // fitted to fight, and a carrier with fighters/drones out is a real screen.
        using namespace EVEDB::invGroups;
        uint16 preyGroup = enemyBot->GetSelf()->groupID();
        if (preyGroup == Carrier || preyGroup == Supercarrier || preyGroup == Titan)
            theirPower += 4;
        else if (preyGroup == Battleship || preyGroup == BlackOps || preyGroup == Marauder)
            theirPower += 1;
        if (enemyBot->GetDroneSE() != nullptr
            || (enemyBot->IsShipSE() && enemyBot->GetShipSE()->GetActiveFighterCount() > 0))
            theirPower += 3;

        if (ShouldEngage(myPower, theirPower, false)) {
            _log(BOT__TRACE, "PlayerBot %s(%u): hunter engaging %s(%u) — %d vs %d.",
                 m_botName.c_str(), m_botCharID, enemyBot->GetBotName().c_str(),
                 enemyBot->GetBotCharID(), myPower, theirPower);
            // Pre-emptively record the grudge — the hunter declares its enemy.
            UpdateBotStandings(enemyBot, false);
            StartAggressionTimer();   // attacking = flagged, can't leave for a bit
            BroadcastAggression(enemyBot->GetBotCharID());   // show the flashing icon
            // Advanced hunter with a fleet: try to set up a warp-bubble ambush
            // first (drop a bubble to trap the prey, then the fleet warps in).
            TryAmbush(prey);
            // Drone hulls launch their drones when engaging.
            if (GetDroneCapacity() > 0 && m_drones.empty())
                SpawnDrones(0);
            ApplyCombatStyle();
            GetAIMgr()->WakeUp();
            GetAIMgr()->StartAttackCycle(2000);
            GetAIMgr()->Target(prey);
            CallFleetSupport(prey);
        } else {
            _log(BOT__TRACE, "PlayerBot %s(%u): hunter passed on %s(%u) — %d vs %d.",
                 m_botName.c_str(), m_botCharID, enemyBot->GetBotName().c_str(),
                 enemyBot->GetBotCharID(), myPower, theirPower);
        }
    }

    // No prey worth engaging (or none found): a hunter doesn't hover in one spot.
    // Between sweeps it drifts to another part of the system (or leaves if this
    // system is dead). Avoid the gate — sweep toward an anomaly/belt instead.
    if (!GetAIMgr()->IsFighting() && !m_destiny->IsWarping() && !m_destiny->IsMoving()) {
        if (MakeRandomInt(0, 99) < 55) {
            PatrolForIdle();
        } else if (MakeRandomInt(0, 99) < 25) {
            // Nothing worth hunting here — move on to greener systems.
            MarkForTravel();
        }
    }
}

void PlayerBot::RatForTarget()
{
    // Peaceful PvE ratter: engage NPC red crosses only (never players/bots).
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (GetAIMgr()->IsFighting())
        return;

    // Look for an NPC in our bubble (or the system) that is a rat.
    SystemBubble* bubble = SysBubble();
    if (bubble == nullptr)
        return;
    std::map<uint32, SystemEntity*> entities;
    bubble->GetAllEntities(entities);
    for (auto& [id, se] : entities) {
        if (se == nullptr || !se->IsNPCSE())
            continue;
        if (se == this || se->GetNPCSE() == this)
            continue;
        // Skip other PlayerBots (they're NPCSE too) — we only shoot real rats.
        if (dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
            continue;
        double d = GetPosition().distance(se->GetPosition());
        if (d > 100000)
            continue;
        // Engage the rat.
        _log(BOT__TRACE, "PlayerBot %s(%u): ratter engaging NPC %s(%u).",
             m_botName.c_str(), m_botCharID, se->GetName(), se->GetID());
        GetAIMgr()->WakeUp();
        GetAIMgr()->StartAttackCycle(2000);
        GetAIMgr()->Target(se);
        return;
    }

    // No rat in this bubble. A real ratter doesn't sit at the gate waiting for
    // rats to come to it — it warps to an anomaly/belt and rats there. Find one
    // in the system (any NPCSE away from us) and fly to it.
    if (!m_destiny->IsWarping() && !m_destiny->IsMoving()) {
        SystemEntity* ratSpot = nullptr;
        double bestD = 0.0;
        for (auto& [id, se] : SystemMgr()->GetEntities()) {
            if (se == nullptr || se == this || se->GetNPCSE() == nullptr)
                continue;
            if (dynamic_cast<PlayerBot*>(se->GetNPCSE()) != nullptr)
                continue;
            // Prefer an NPC cluster (an anomaly spawn) over a lone far one.
            double d = GetPosition().distance(se->GetPosition());
            if (d > 250000 && d > bestD) { bestD = d; ratSpot = se; }
        }
        if (ratSpot != nullptr) {
            m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
            m_destiny->WarpTo(ratSpot->GetPosition(), 2000);
            _log(BOT__TRACE, "PlayerBot %s(%u): no rats in bubble — warping to %s(%u) to rat.",
                 m_botName.c_str(), m_botCharID, ratSpot->GetName(), ratSpot->GetID());
        }
    }
}

void PlayerBot::ClaimSystem()
{
    // PvP war corps skirmish: claim an unowned nullsec system for the corp.
    // Only corps inside an alliance can claim sovereignty.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (m_botAllianceID == 0) {
        _log(BOT__TRACE, "PlayerBot %s(%u): no alliance — cannot claim sovereignty.",
             m_botName.c_str(), m_botCharID);
        return;
    }
    uint32 sysID = SystemMgr()->GetID();

    // Only claim if the system isn't already owned.
    SovereigntyData sov = svDataMgr.GetSovereigntyData(sysID);
    if (sov.allianceID != 0 || sov.corporationID != 0)
        return;   // already claimed

    // Only occasionally (not every tick), and require a practised attacker.
    float practice = m_memory ? m_memory->GetActivitySkill() : 0.0f;
    if (MakeRandomInt(0, 999) >= (int)(10 + practice * 40))
        return;

    SovereigntyData claim = SovereigntyData();
        claim.solarSystemID = sysID;
        claim.constellationID = SystemMgr()->GetConstellationID();
        claim.regionID = SystemMgr()->GetRegionID();
        claim.corporationID = m_botCorpID;
        claim.allianceID = m_botAllianceID;
        claim.claimStructureID = GetID();
        claim.claimTime = (int64)GetFileTimeNow();
        claim.contested = 0;
    svDataMgr.AddSovClaim(claim);

    _log(BOT__MESSAGE, "PlayerBot %s(%u): nullsec skirmish — claiming system %u for corp %u.",
         m_botName.c_str(), m_botCharID, sysID, m_botCorpID);
}

void PlayerBot::RequestFleetProtection()
{
    // Cooperative behaviour: have same-corp combat pilots (fighter/support)
    // cover this industrial bot while it mines/hauls. Guards fly to us and stay.
    if (SystemMgr() == nullptr)
        return;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* guard = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (guard == nullptr || guard == this)
            continue;
        if (guard->GetBotCorpID() != m_botCorpID && guard->GetBotAllianceID() != m_botAllianceID)
            continue;   // only corpmates guard
        if (guard->GetProfession() != BotProfession::Hunter)
            continue;   // only combat-profession bots are guards
        if (guard->GetAIMgr()->IsFighting() || guard->IsTraveling())
            continue;
        if (guard->GetPosition().distance(GetPosition()) > 50000 && MakeRandomInt(0, 99) < 50) {
            guard->GetAIMgr()->WakeUp();
            guard->DestinyMgr()->SetMaxVelocity(guard->GetAIMgr()->GetMaxShipSpeed());
            guard->DestinyMgr()->WarpTo(GetPosition(), 1500);
            _log(BOT__TRACE, "PlayerBot %s(%u): guard %s(%u) covering industrial.",
                 m_botName.c_str(), m_botCharID, guard->GetBotName().c_str(), guard->GetBotCharID());
        }
    }
}

void PlayerBot::StartJumpFreighter(uint32 destSystem)
{
    // Big-cargo courier: light a cyno, hold an interception window, then jump.
    if (destSystem == 0)
        return;
    m_isJumpFreighter = true;
    m_cynoActive = true;
    m_jumpDest = destSystem;
    // 60-120s window: players can warp in, shoot the freighter, or its guards.
    m_cynoTimer.Start(MakeRandomInt(60000, 120000));

    // Visible cyno effect (effects.CynosuralGeneration) so players see the beacon.
    if (m_destiny != nullptr)
        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                     m_self->itemID(), 0, "effects.CynosuralGeneration",
                                     1, 1, 1, 60000, 0, 0);
    // Guards come protect the freighter while it lights the cyno.
    RequestFleetProtection();

    _log(BOT__MESSAGE, "PlayerBot %s(%u): JUMP FREIGHTER — cyno lit, jumping to %u in %.0fs.",
         m_botName.c_str(), m_botCharID, destSystem, m_cynoTimer.GetRemainingTime() / 1000.0);
}

void PlayerBot::UseCombatAbilities()
{
    // Use the bot's full arsenal while fighting: logistics repair allies,
    // commanders apply gang bonuses, support relies on NPCAI EWAR modules
    // (web/scram/ECM/paint handled in AttackTarget) and fighters just DPS.
    if (m_role == BotRole::Logistics) {
        // Find the most damaged ally in system and remote-repair it.
        PlayerBot* patient = nullptr;
        float lowestHullPct = 2.0f;
        for (auto& [id, se] : SystemMgr()->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* ally = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (ally == nullptr || ally == this)
                continue;   // a logi doesn't remote-rep itself
            if (ally->GetBotCorpID() != m_botCorpID && ally->GetBotAllianceID() != m_botAllianceID)
                continue;
            InventoryItemRef aSelf = ally->GetSelf();
            float cap = aSelf->GetAttribute(AttrShieldCapacity).get_float();
            float cur = aSelf->GetAttribute(AttrShieldCharge).get_float();
            float pct = (cap > 0) ? (cur / cap) : 1.0f;
            if (pct < lowestHullPct) { lowestHullPct = pct; patient = ally; }
        }
        if (patient != nullptr) {
            // Remote shield boost (logi repper).
            InventoryItemRef pSelf = patient->GetSelf();
            float boost = 250.0f * (1.0f + m_botSkill * 0.2f);
            float newShield = pSelf->GetAttribute(AttrShieldCharge).get_float() + boost;
            if (newShield > pSelf->GetAttribute(AttrShieldCapacity).get_float())
                newShield = pSelf->GetAttribute(AttrShieldCapacity).get_float();
            pSelf->SetAttribute(AttrShieldCharge, newShield);
            patient->SendDamageStateChanged();
            m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                         patient->GetID(), 0, "effects.ShieldBoosting",
                                         0, 1, 1, 5000, 0, 0);
            _log(BOT__TRACE, "PlayerBot %s(%u): logistic — remote repping %s(%u).",
                 m_botName.c_str(), m_botCharID, patient->GetBotName().c_str(), patient->GetBotCharID());
        }
    } else if (m_role == BotRole::Commander) {
        // Gang bonus: apply a damage/agility boost to all fleet members.
        float dmgBonus = 1.0f + 0.05f * (1.0f + m_botSkill * 0.1f);   // +5..10%
        for (auto& [id, se] : SystemMgr()->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* ally = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (ally == nullptr || ally == this)
                continue;
            if (ally->GetBotCorpID() != m_botCorpID && ally->GetBotAllianceID() != m_botAllianceID)
                continue;
            if (ally->GetSelf()->HasAttribute(AttrDamageMultiplier))
                ally->GetSelf()->SetAttribute(AttrDamageMultiplier,
                    ally->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() * dmgBonus, false);
        }
        _log(BOT__TRACE, "PlayerBot %s(%u): commander — fleet bonus applied.",
             m_botName.c_str(), m_botCharID);
    }
    // Support role: EWAR modules (web/scram/ECM/paint) are fired by NPCAI's
    // AttackTarget() already; Fighter: pure DPS via NPCAI. Nothing extra here.
    // — but before the turn ends, read the battlefield and re-aim if needed.
    AnalyzeCombatSituation();
}

void PlayerBot::AnalyzeCombatSituation()
{
    // Called every ~5s while fighting. Reads the fight and does two things a
    // real pilot does: focus fire on the most valuable enemy (logi/commander/
    // EWAR die first) and disengage when the fight is clearly lost.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    if (m_killed)
        return;
    if (!GetAIMgr()->IsFighting())
        return;

    // 1) Re-target toward the priority enemy. Support ships in particular want
    //    the enemy commander/logi locked so their EWAR (web/scram/ECM/paint)
    //    lands on the ship that matters most, not whoever locked them first.
    if (m_role == BotRole::Support || m_role == BotRole::Commander) {
        SystemEntity* prio = PickPriorityTarget(nullptr);
        if (prio != nullptr && prio->DestinyMgr() != nullptr) {
            SystemEntity* cur = m_targMgr != nullptr ? m_targMgr->GetFirstTarget() : nullptr;
            if (cur != prio) {
                _log(BOT__TRACE, "PlayerBot %s(%u): re-targeting %s(%u) — priority.",
                     m_botName.c_str(), m_botCharID, prio->GetName(), prio->GetID());
                GetAIMgr()->Target(prio);
            }
        }
    }

    // 2) Disengage check — don't throw the ship away.
    //    a) own hull critically low (< ~35% pooled shield+armor+structure);
    //    b) badly outnumbered (enemies - allies >= 3) while already hurt.
    InventoryItemRef self = GetSelf();
    float shieldCap = self->GetAttribute(AttrShieldCapacity).get_float();
    float shieldCur = self->GetAttribute(AttrShieldCharge).get_float();
    float armorHP   = self->GetAttribute(AttrArmorHP).get_float();
    float armorDmg  = self->GetAttribute(AttrArmorDamage).get_float();
    float hullHP    = self->GetAttribute(AttrHP).get_float();
    float hullDmg   = self->GetAttribute(AttrDamage).get_float();
    float maxHP = shieldCap + armorHP + hullHP;
    float curHP = shieldCur + (armorHP - armorDmg) + (hullHP - hullDmg);
    float hpPct = maxHP > 0.0f ? (curHP / maxHP) : 1.0f;

    int allies = 0, enemies = 0;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (other == nullptr || other == this)
            continue;
        if (other->GetBotCorpID() == m_botCorpID || other->GetBotAllianceID() == m_botAllianceID)
            ++allies;
        else if (other->GetAIMgr()->IsFighting())
            ++enemies;
    }

    bool outnumbered = (enemies - allies) >= 3 && hpPct < 0.55f;
    if (hpPct < 0.35f || outnumbered) {
        _log(BOT__TRACE, "PlayerBot %s(%u): disengaging (hp=%.0f%%, enemies=%d, allies=%d).",
             m_botName.c_str(), m_botCharID, hpPct * 100.0f, enemies, allies);
        RecallDrones();
        GetAIMgr()->StartAttackCycle(0);
        // Flee needs a tracked threat; use the current target, else any fighting enemy.
        SystemEntity* fleeFrom = m_targMgr != nullptr ? m_targMgr->GetFirstTarget() : nullptr;
        if (fleeFrom == nullptr) {
            for (auto& [id, se] : SystemMgr()->GetEntities()) {
                if (se == nullptr || se->GetNPCSE() == nullptr)
                    continue;
                PlayerBot* other = dynamic_cast<PlayerBot*>(se->GetNPCSE());
                if (other == nullptr || other == this)
                    continue;
                if (other->GetBotCorpID() == m_botCorpID || other->GetBotAllianceID() == m_botAllianceID)
                    continue;
                if (other->GetAIMgr()->IsFighting()) { fleeFrom = se; break; }
            }
        }
        if (fleeFrom != nullptr)
            GetAIMgr()->Flee(fleeFrom);
    }
}

// Drop a Mobile Warp Disruptor bubble at pos. Uses the same DeployableSE
// machinery as a player anchoring an MWD: the deployable is online immediately,
// sets the bubble's warp-disruption flag, and its ScrambleCheck (DeployableSE::
// Process) applies AttrWarpScrambleStatus to any ship inside the bubble, so the
// trapped target can't warp out.
void PlayerBot::DeployWarpBubble(const GPoint& pos)
{
    if (SystemMgr() == nullptr || m_destiny == nullptr)
        return;
    if (pos.isZero() || pos.isNaN() || pos.isInf())
        return;

    // Mobile Medium Warp Disruptor I (12199) — 11.5km bubble, good ambush size.
    const uint32 mwdTypeID = 12199;
    ItemData idata(mwdTypeID, m_botCharID, SystemMgr()->GetID(), flagNone, "Warp Bubble", pos);
    InventoryItemRef ref = sItemFactory.SpawnItem(idata);
    if (ref.get() == nullptr)
        return;
    ref->ChangeSingleton(true);
    ref->SetPosition(pos);

    FactionData data = FactionData();
    data.corporationID = m_botCorpID;
    data.ownerID = m_botCharID;
    data.allianceID = m_botAllianceID;
    DeployableSE* dSE = new DeployableSE(ref, SystemMgr()->GetServiceMgr(), SystemMgr(), data);
    if (dSE == nullptr) { ref->Delete(); return; }
    dSE->SetPosition(pos);
    SystemMgr()->AddEntity(dSE);
    dSE->SetImmediateOnline();   // anchored + online instantly; sets bubble warp flag, starts scramble timer

    // Visible WarpDisruptFieldGenerating effect to everyone in the bubble (the
    // Process timer path would send it on the next tick, but players should SEE
    // the bubble right away).
    if (dSE->DestinyMgr() != nullptr && dSE->SysBubble() != nullptr) {
        OnSpecialFX14 fx;
            fx.entityID = ref->itemID();
            fx.moduleID = ref->itemID();
            fx.moduleTypeID = mwdTypeID;
            fx.targetID = PyStatic.NewNone();
            fx.chargeTypeID = PyStatic.NewNone();
            fx.area = new PyList();
            fx.guid = "effects.WarpDisruptFieldGenerating";
            fx.isOffensive = 0;
            fx.start = 1;
            fx.active = 1;
            fx.duration = -1;
            fx.repeat = 0;
            fx.startTime = GetFileTimeNow();
            PyDict* gd = new PyDict();
            gd->SetItemString("range", new PyFloat(11500.0f));
            fx.graphicInfo = new PyObject("util.KeyVal", gd);
        PyTuple* payload = fx.Encode();
        dSE->DestinyMgr()->SendSingleDestinyUpdate(&payload);
    }
    _log(BOT__TRACE, "PlayerBot %s(%u): deployed warp bubble at (%.0f,%.0f,%.0f).",
         m_botName.c_str(), m_botCharID, pos.x, pos.y, pos.z);
}

// Advanced hunters set up warp-bubble ambushes: drop a bubble near the target
// (traps it so it can't warp), call the fleet, then engage. Only experienced
// pilots (skill tier >= 3) with allies nearby pull this off, and not every time.
bool PlayerBot::TryAmbush(SystemEntity* target)
{
    if (target == nullptr || target->GetSelf().get() == nullptr || m_destiny == nullptr)
        return false;
    if (m_botSkill < 3)                 // ambushes are for experienced pilots
        return false;
    if (CountAlliesNearby() < 1)        // a solo bubble is just a target's gift
        return false;
    if (target->GetPosition().distance(GetPosition()) > 60000)
        return false;                   // don't chase across the system to drop a bubble
    if (MakeRandomInt(0, 99) >= 20)     // sometimes it just attacks, no bubble
        return false;

    // If the target is already inside a warp bubble, no need for another.
    if (target->SysBubble() != nullptr && target->SysBubble()->HasWarpBubble())
        return false;

    DeployWarpBubble(target->GetPosition());
    CallFleetSupport(target);           // the ambush fleet warps in
    StartAggressionTimer();
    _log(BOT__TRACE, "PlayerBot %s(%u): warp-bubble ambush on %s(%u).",
         m_botName.c_str(), m_botCharID, target->GetName(), target->GetID());
    return true;
}
