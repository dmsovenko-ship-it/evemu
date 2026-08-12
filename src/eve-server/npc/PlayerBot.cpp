#include "eve-server.h"
#include "npc/PlayerBot.h"
#include "npc/NPCAI.h"
#include "npc/Drone.h"
#include "system/Damage.h"
#include "Client.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/BeltMgr.h"
#include "system/Asteroid.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/sov/SovereigntyDataMgr.h"
#include "standing/StandingDB.h"
#include <iterator>
#include <cmath>

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
  m_cynoTimer(0)
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

    _log(BOT__TRACE, "PlayerBot %s(%u): attacked by %s — sysSec %.1f mayAttack %s, myPower %d vs theirPower %d%s.",
         m_botName.c_str(), m_botCharID, attacker->GetName(), sysSec, mayAttack?"yes":"no",
         myPower, theirPower, isBait ? " [BAIT]" : "");

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
        default:
            return 2;   // unknown / industrial-ish — treat as cruiser-ish
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
    m_wantsTravel = true;
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
        // PvE rat hunter also learns from a rat kill.
        if (m_profession == BotProfession::RatHunter && m_memory) {
            m_memory->RecordRatKill();
            m_memory->Save();
        }
        _log(BOT__TRACE, "PlayerBot %s(%u): fight ended, recorded win.", m_botName.c_str(), m_botCharID);
    } else if (fighting) {
        m_inFight = true;
    }

    // Profession learning: each completed activity run adds experience so the bot
    // gets better at its job (see GetActivitySkill).
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
    if (!m_decisionTimer.Enabled())
        m_decisionTimer.Start(15000);   // re-evaluate every ~15s
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
    NPC::Killed(damage);
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

    // Novices make mistakes: even when they "should" win they sometimes misjudge
    // and flee; even when they should lose they sometimes overcommit. Veterans
    // (skill ~1) are reliable.
    float mistakeChance = 0.30f * (1.0f - skill);
    if (mistakeChance > 0.0f && MakeRandomFloat() < mistakeChance) {
        if (m_memory) m_memory->RecordPvpMistake();
        wouldWin = !wouldWin;   // a misread — acts on the wrong call
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
                                               "effects.Laser", 1, 1, 1, 2000, 0, 0);
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
        if (drone == nullptr || drone->DestinyMgr() == nullptr) {
            it = m_drones.erase(it);
            continue;
        }
        double distToBot = drone->GetPosition().distance(GetPosition());
        if (distToBot > 30000 || drone->IsPendingRemoval() || drone->IsEnabled() == false) {
            // scoop / cleanup
            drone->DestinyMgr()->Stop();
            if (drone->SysBubble() != nullptr)
                SystemMgr()->RemoveEntity(drone);
            drone->GetSelf()->Delete();
            delete drone;
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
        if (drone->DestinyMgr() != nullptr)
            drone->DestinyMgr()->Stop();
        if (drone->SysBubble() != nullptr && SystemMgr() != nullptr)
            SystemMgr()->RemoveEntity(drone);
        drone->GetSelf()->Delete();
        delete drone;
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

void PlayerBot::DoProfessionActivity()
{
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;

    // Self-learning: practiced bots act more often / more efficiently.
    float practice = m_memory ? m_memory->GetActivitySkill() : 0.0f;
    float chanceBoost = 30.0f + practice * 40.0f;   // 30%..70% action chance

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
        } break;

        case BotProfession::Miner: {
            // Peaceful miner: fly/warp to a belt asteroid and sit mining. In a fleet
            // (same corp) miners cooperate at one belt; guard fighters protect them.
            BeltMgr* beltMgr = SystemMgr()->GetBeltMgr();
            if (beltMgr != nullptr) {
                AsteroidSE* roid = beltMgr->GetAnyAsteroid();
                if (roid != nullptr && roid->DestinyMgr() != nullptr && !m_destiny->IsWarping()) {
                    m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed() / 2);
                    m_destiny->WarpTo(roid->GetPosition(), 1000);
                    _log(BOT__TRACE, "PlayerBot %s(%u): mining — warping to asteroid %u.",
                         m_botName.c_str(), m_botCharID, roid->GetID());
                }
            }
            // Cooperative mining: ask corpmates (guards) to cover this miner.
            RequestFleetProtection();
            // End of mining run: head to the station to refine the ore. Experienced
            // miners haul more ore per trip (self-learning), so longer runs.
            float runLen = 2.0f + practice * 6.0f;   // 2..8 trips between docks
            if (m_mineTrips >= runLen) {
                m_mineTrips = 0;
                RequestDock();
                _log(BOT__TRACE, "PlayerBot %s(%u): ore hold full — docking to refine/sell.",
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
            // Peaceful hacker: warp to an anomaly site (data/relic) and sit there.
            // Experienced hackers run sites more frequently (self-learning).
            if (SystemMgr()->GetAnomMgr() != nullptr && MakeRandomInt(0, 99) < (int)chanceBoost) {
                // AnomalyMgr has signatures; we just drift toward a random one.
                MarkForTravel();   // simulate running sites between systems
            }
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
        // Standing at the site: "scanning" earns loot. Record the run.
        if (m_memory && MakeRandomInt(0, 99) < 20) {
            m_memory->RecordHackRun();
            m_memory->Save();
        }
        // After several scans, haul the loot back to the hub to sell.
        if (m_mineTrips++ > 5) {
            m_mineTrips = 0;
            RequestDock();   // go home, sell the loot
        }
        return;
    }
    // No sites here — move on (explore another system).
    if (MakeRandomInt(0, 99) < 40)
        MarkForTravel();
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

        if (ShouldEngage(myPower, theirPower, false)) {
            _log(BOT__TRACE, "PlayerBot %s(%u): hunter engaging %s(%u) — %d vs %d.",
                 m_botName.c_str(), m_botCharID, enemyBot->GetBotName().c_str(),
                 enemyBot->GetBotCharID(), myPower, theirPower);
            // Pre-emptively record the grudge — the hunter declares its enemy.
            UpdateBotStandings(enemyBot, false);
            StartAggressionTimer();   // attacking = flagged, can't leave for a bit
            BroadcastAggression(enemyBot->GetBotCharID());   // show the flashing icon
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
            if (ally == nullptr)
                continue;
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
}
