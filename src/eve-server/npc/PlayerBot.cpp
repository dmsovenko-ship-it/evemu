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
