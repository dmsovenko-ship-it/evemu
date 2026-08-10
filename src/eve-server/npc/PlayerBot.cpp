#include "eve-server.h"
#include "npc/PlayerBot.h"
#include "npc/NPCAI.h"
#include "Client.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/BeltMgr.h"
#include "system/Asteroid.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include <iterator>

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
  m_profession(BotProfession::Miner),
  m_memory(std::make_unique<BotMemory>(charID)),
  m_decisionTimer(0),
  m_travelTimer(0),
  m_wantsTravel(false),
  m_traveling(false),
  m_abilityTimer(0),
  m_inFight(false)
{
    // A player-like legend: give this NPC a neutral alliance so it doesn't show
    // red crosshairs and isn't auto-aggroed by faction standing checks.
    m_allyID = m_botAllianceID;
    m_corpID = m_botCorpID;
    m_warID = 0;
    m_ownerID = m_botCorpID;

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
        slim->SetItemString("securityStatus", new PyFloat(0.0f));
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

    // Combat power = ship class + skill tier (0..5). A skilled pilot in a
    // weaker hull can beat a rookie in a bigger one — but usually not by much.
    int myPower = myClass * 2 + (int)m_botSkill;
    int theirPower = attackerClass * 2 + (sConfig.playerBots.AggroFactor > 0 ? 1 : 0);
    // AggroFactor: % confidence modifier. +10% → treat enemy as 10% weaker, etc.
    theirPower -= (int)(theirPower * (sConfig.playerBots.AggroFactor / 100.0f));

    // Self-learning: a bot with a history of wins fights more boldly; a bot that
    // keeps losing is cautious. GetAggression() ∈ [-1..+1] shifts the balance.
    float learned = m_memory ? m_memory->GetAggression() : 0.0f;
    myPower += (int)(learned * 2.0f);   // ±2 combat power from experience

    _log(BOT__TRACE, "PlayerBot %s(%u): attacked by %s — sysSec %.1f mayAttack %s, myPower %d vs theirPower %d.",
         m_botName.c_str(), m_botCharID, attacker->GetName(), sysSec, mayAttack?"yes":"no", myPower, theirPower);

    if (mayAttack && myPower >= theirPower - 1) {
        // Legal and confident — fight back (NPCAI handles targeting/attack).
        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxShipSpeed());
        GetAIMgr()->WakeUp();
        GetAIMgr()->StartAttackCycle(2000);
        // Intelligent fleet support: call allies (same corp or same alliance)
        // in this system to join the fight, so fights are decided by force
        // concentration, not one brave pilot.
        CallFleetSupport(attacker);
    } else {
        // Not legal to fight, or outmatched — flee. A professional pilot warps
        // out rather than suicide against a superior (or unlawful) force.
        _log(BOT__TRACE, "PlayerBot %s(%u): fleeing (%s).",
             m_botName.c_str(), m_botCharID, mayAttack ? "outmatched" : "no kill right");
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

void PlayerBot::MarkForTravel()
{
    if (m_traveling || m_wantsTravel)
        return;
    // Visibly warp to a gate in this system. The travel timer covers the flight
    // time (a few AU at warp speed); when it expires the bot is gone from here.
    // The bot does NOT get deleted at the moment it decides — it first flies.
    m_traveling = true;
    m_travelTimer.Start(MakeRandomInt(12000, 20000));   // visible warp to gate

    // Warp toward a random gate in the system so the player sees the ship leave.
    auto gates = SystemMgr()->GetGates();
    if (!gates.empty()) {
        auto it = gates.begin();
        std::advance(it, MakeRandomInt(0, (int64)gates.size() - 1));
        if (it->second != nullptr) {
            GPoint gatePos = it->second->GetPosition();
            DestinyMgr()->WarpTo(gatePos, 1000);
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
        _log(BOT__TRACE, "PlayerBot %s(%u): fight ended, recorded win.", m_botName.c_str(), m_botCharID);
    } else if (fighting) {
        m_inFight = true;
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
    NPC::Killed(damage);
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

    switch (m_profession) {
        case BotProfession::Hunter: {
            // PvP pirate: hunt for legal PvP targets in lowsec/nullsec.
            HuntForTarget();
        } break;

        case BotProfession::RatHunter: {
            // Peaceful PvE: only engage NPC red crosses (ratting), never players.
            RatForTarget();
        } break;

        case BotProfession::Miner: {
            // Peaceful miner: warp to a belt asteroid and sit mining. In a fleet
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
        } break;

        case BotProfession::Trader:
        case BotProfession::Courier: {
            // Peaceful trader/courier: occasionally move between stations/gates.
            for (auto& [id, se] : SystemMgr()->GetStaticEntities()) {
                if (se != nullptr && (se->GetStationSE() != nullptr || se->GetGateSE() != nullptr)) {
                    if (!m_destiny->IsWarping() && MakeRandomInt(0, 99) < 40) {
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
            if (SystemMgr()->GetAnomMgr() != nullptr && MakeRandomInt(0, 99) < 40) {
                // AnomalyMgr has signatures; we just drift toward a random one.
                MarkForTravel();   // simulate running sites between systems
            }
        } break;
    }
}

void PlayerBot::HuntForTarget()
{
    // Aggressive (hunter / PvP war corp): actively seek a legal target.
    // Legal = in lowsec/nullsec any pilot; in highsec only criminals/low-sec.
    if (m_destiny == nullptr || SystemMgr() == nullptr)
        return;
    float sysSec = SystemMgr()->GetSystemSecurityRating();

    // Only hunt where PvP is viable (lowsec/nullsec mostly; highsec rarely).
    if (sysSec >= 0.5f && MakeRandomInt(0, 99) >= 5)
        return;

    // Find a target: enemy PlayerBots in this system that are fighting or idle.
    SystemEntity* prey = nullptr;
    int bestScore = 0;
    for (auto& [id, se] : SystemMgr()->GetEntities()) {
        if (se == nullptr || se->GetNPCSE() == nullptr)
            continue;
        PlayerBot* enemy = dynamic_cast<PlayerBot*>(se->GetNPCSE());
        if (enemy == nullptr || enemy == this)
            continue;
        if (enemy->GetBotCorpID() == m_botCorpID || enemy->GetBotAllianceID() == m_botAllianceID)
            continue;   // ally
        // Only hunt other aggressive corps' bots or any bot in null — keep it
        // from ganking peaceful miners constantly.
        if (sysSec >= 0.5f && !enemy->IsAggressive())
            continue;   // highsec: only hunt aggressive targets (both flagged)
        double d = GetPosition().distance(enemy->GetPosition());
        if (d > 100000)
            continue;   // out of hunt range
        int score = (int)(100000 - d) / 1000;
        if (score > bestScore) { bestScore = score; prey = enemy; }
    }

    // If a suitable prey is found, evaluate the fight and engage if favoured.
    if (prey != nullptr) {
        PlayerBot* enemyBot = (PlayerBot*)prey;
        int myClass = GetShipClass(m_self->groupID());
        int enemyClass = GetShipClass(enemyBot->GetSelf()->groupID());
        int myPower = myClass * 2 + (int)m_botSkill;
        int theirPower = enemyClass * 2 + (int)enemyBot->GetBotSkillLevel();
        if (m_memory)
            myPower += (int)(m_memory->GetAggression() * 2.0f);
        if (myPower >= theirPower) {
            _log(BOT__TRACE, "PlayerBot %s(%u): hunter engaging %s(%u).",
                 m_botName.c_str(), m_botCharID, enemyBot->GetBotName().c_str(), enemyBot->GetBotCharID());
            GetAIMgr()->WakeUp();
            GetAIMgr()->StartAttackCycle(2000);
            GetAIMgr()->Target(prey);
            CallFleetSupport(prey);
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

void PlayerBot::UseCombatAbilities()
{
    // Use the bot's full arsenal while fighting: logistics repair allies,
    // commanders apply gang bonuses, support relies on NPCAI EWAR modules
    // (web/scram/ECM/paint handled in AttackTarget) and fighters just DPS.    if (m_role == BotRole::Logistics) {
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
