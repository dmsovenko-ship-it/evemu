#include "eve-server.h"
#include "npc/PlayerBot.h"
#include "Client.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
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
  m_decisionTimer(0),
  m_travelTimer(0),
  m_wantsTravel(false),
  m_traveling(false)
{
    // A player-like legend: give this NPC a neutral alliance so it doesn't show
    // red crosshairs and isn't auto-aggroed by faction standing checks.
    m_allyID = m_botAllianceID;
    m_corpID = m_botCorpID;
    m_warID = 0;
    m_ownerID = m_botCorpID;
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
    if (attacker->GetSelf() != nullptr) {
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

    _log(BOT__TRACE, "PlayerBot %s(%u): attacked by %s — sysSec %.1f mayAttack %s, myPower %d vs theirPower %d.",
         m_botName.c_str(), m_botCharID, attacker->GetName(), sysSec, mayAttack?"yes":"no", myPower, theirPower);

    if (mayAttack && myPower >= theirPower - 1) {
        // Legal and confident — fight back (NPCAI handles targeting/attack).
        m_destiny->SetMaxVelocity(GetAIMgr()->GetMaxSpeed());
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
    // system. They join the fight (Target() starts targeting+attack) so a fleet
    // concentrates force — mirroring real EVE blobs.
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
        // Only rally if the ally isn't already busy fleeing.
        if (ally->GetAIMgr()->IsFighting())
            continue;
        _log(BOT__TRACE, "PlayerBot %s(%u): fleet support — %s(%u) joining.",
             m_botName.c_str(), m_botCharID, ally->GetBotName().c_str(), ally->GetBotCharID());
        ally->GetAIMgr()->WakeUp();
        ally->GetAIMgr()->StartAttackCycle(2000);
        ally->GetAIMgr()->Target(attacker);
    }
}

// Ship class by groupID: bigger index = more combat power.
int PlayerBot::GetShipClass(uint16 groupID)
{
    using namespace EVEDB::invGroups;
    switch (groupID) {
        case Frigate: case Assault_Frigate: case Interceptor:
        case Covert_Ops: case Electronic_Attack_Ship:
        case Interdictor: case Stealth_Bomber:
            return 1;
        case Destroyer: case Interdictor_2: case Interdictor_3:
            return 2;
        case Cruiser: case Heavy_Assault_Cruiser: case Heavy_Interceptor:
        case Logistics_Frigate: case Combat_Recon_Ship:
            return 3;
        case Battlecruiser: case Command_Ship: case Strategic_Cruiser:
        case Force_Recon_Ship: case Logistics_Cruiser:
            return 4;
        case Battleship: case Black_Ops: case Marauder:
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

    if (m_decisionTimer.Check(false)) {
        DecideNextAction();
    }
    if (!m_decisionTimer.Enabled())
        m_decisionTimer.Start(15000);   // re-evaluate every ~15s
}

void PlayerBot::DecideNextAction()
{
    // Hook for BotMgr. Placeholder for the state machine that will be wired
    // to actual travel/combat/mining behaviour.
    _log(BOT__TRACE, "PlayerBot %s(%u): activity = %u, system = %u",
         m_botName.c_str(), m_botCharID, (uint8)m_activity, SystemMgr()->GetID());
}
