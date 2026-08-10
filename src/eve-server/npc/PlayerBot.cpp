#include "eve-server.h"
#include "npc/PlayerBot.h"
#include "Client.h"
#include "system/SystemBubble.h"

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
  m_decisionTimer(0)
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

void PlayerBot::Process()
{
    // Rides NPC::Process for movement/destiny/target handling, but the bot's
    // activities (travel, dock, rat, mine, flee) are orchestrated by BotMgr,
    // which calls DecideNextAction() and issues Destiny commands directly.
    NPC::Process();

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
