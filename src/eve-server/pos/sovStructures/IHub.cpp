
/**
 * @name IHub.cpp
 *   Class for Infrastructure Hubs.
 *
 * @Author:         James
 * @date:   8 April 2021
 */

/*
 * POS__ERROR
 * POS__WARNING
 * POS__MESSAGE
 * POS__DUMP
 * POS__DEBUG
 * POS__DESTINY
 * POS__SLIMITEM
 * POS__TRACE
 */


#include "Client.h"
#include "EntityList.h"
#include "EVEServerConfig.h"
#include "planet/Planet.h"
#include "pos/sovStructures/IHub.h"
#include "system/Container.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

IHubSE::IHubSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& fData)
: StructureSE(structure, services, system, fData),
  m_reinforceHour(MakeRandomInt(0, 23)) // random default exit hour
{
}


void IHubSE::Init()
{
    _log(SE__TRACE, "IHubSE %s(%u) is being initialised", m_self->name(), m_self->itemID());
    StructureSE::Init();

    // check for valid bubble
    if (m_bubble == nullptr)
        assert(0);
    m_bubble->SetIHubSE(this);

    // set global attribute
    m_self->SetAttribute(AttrIsGlobal, EvilOne, false);
}

void IHubSE::Process()
{
    // Process base class state timers
    StructureSE::Process();

    // Handle IHub reinforcement state transitions
    if (m_procState == EVEPOS::ProcState::Reinforcing) {
        // Shield reinforcement expired → move to armor-reinforced
        _log(POS__MESSAGE, "IHub %s(%u): Shield reinforcement expired, entering Armor Reinforced.", GetName(), m_data.itemID);
        m_data.state = EVEPOS::StructureState::ArmorReinforced;
        m_procState = EVEPOS::ProcState::ArmorReinforcing;
        m_self->SetFlag(flagStructureInactive);
        SetTimer(48 * EvE::Time::Hour * 1000); // 48h armor reinforcement
        m_db.UpdateBaseData(m_data);
        SendSlimUpdate();
    }
    if (m_procState == EVEPOS::ProcState::ArmorReinforcing) {
        // Armor reinforcement expired → become vulnerable
        _log(POS__MESSAGE, "IHub %s(%u): Armor reinforcement expired, entering Vulnerable.", GetName(), m_data.itemID);
        m_data.state = EVEPOS::StructureState::Vulnerable;
        m_procState = EVEPOS::ProcState::Operating;
        m_self->SetFlag(flagStructureActive);
        m_db.UpdateBaseData(m_data);
        SendSlimUpdate();
    }
}

void IHubSE::SetReinforce(EVEPOS::ProcState pState)
{
    _log(POS__MESSAGE, "IHub %s(%u): Entering reinforcement (state %i, reinforceHour=%i).",
         GetName(), m_data.itemID, pState, m_reinforceHour);
    m_procState = pState;
    switch (pState) {
        case EVEPOS::ProcState::Reinforcing:
            m_data.state = EVEPOS::StructureState::SheildReinforced;
            break;
        case EVEPOS::ProcState::ArmorReinforcing:
            m_data.state = EVEPOS::StructureState::ArmorReinforced;
            break;
        default:
            return;
    }

    // Calculate timer: 24h base + alignment to reinforceHour (+/- 3h variance)
    // Timer ends at: next occurrence of reinforceHour (±3h) after 24h from now
    int64 now = GetFileTimeNow();
    int64 baseEnd = now + 24 * EvE::Time::Hour * 1000; // 24h from now

    // Calculate seconds until next reinforceHour
    int64 msPerDay = 24 * 60 * 60 * 1000;
    int64 msSinceMidnight = (now / 1000) % (24 * 3600); // seconds since midnight in Win32 time
    int32 targetMs = m_reinforceHour * 3600 * 1000; // target in ms
    int32 msRemaining = targetMs - static_cast<int32>(msSinceMidnight * 1000);
    if (msRemaining <= 0)
        msRemaining += msPerDay;

    // Align: the exit is at reinforceHour ± 3h variance after 24h minimum
    int64 exitTime = baseEnd;
    // Find the next reinforceHour after baseEnd
    int64 msToGo = (exitTime / 1000) % (24 * 3600) * 1000;
    int32 adjust = targetMs - static_cast<int32>(msToGo);
    if (adjust < 0)
        adjust += msPerDay;
    exitTime += adjust;

    // Add variance = ±3h random
    int64 varianceMs = MakeRandomInt(-10800, 10800) * 1000; // ±3h
    exitTime += varianceMs;

    // Minimum: 24h from now
    int64 reinforceDuration = exitTime - now;
    if (reinforceDuration < 24 * EvE::Time::Hour * 1000)
        reinforceDuration = 24 * EvE::Time::Hour * 1000;
    if (reinforceDuration > 30 * EvE::Time::Hour * 1000)
        reinforceDuration = 30 * EvE::Time::Hour * 1000;

    _log(POS__MESSAGE, "IHub %s(%u): Reinforcement timer = %lli ms (%.1f hours). Exit window around %02i:00 ±3h.",
         GetName(), m_data.itemID, reinforceDuration, reinforceDuration / 3600000.0, m_reinforceHour);

    m_self->SetFlag(flagStructureInactive);
    SetTimer(static_cast<uint32>(reinforceDuration / 1000));
    m_db.UpdateBaseData(m_data);
    SendSlimUpdate();

    PyDict* notifData = new PyDict();
    notifData->SetItemString("structureID", new PyInt(m_data.itemID));
    notifData->SetItemString("solarSystemID", new PyInt(m_system->GetID()));
    sEntityList.CreateNotification(m_corpID, Notify::Types::CorpStructLost, m_data.itemID, notifData);
}

void IHubSE::SetOnline()
{
    _log(SOV__DEBUG, "Onlining IHub... Updating claim'd hubID.");
    StructureSE::SetOnline();
    svDataMgr.UpdateSystemHubID(m_self->locationID(), m_self->itemID());
}

void IHubSE::SetOffline()
{
    _log(SOV__DEBUG, "Offlining IHub... Resetting claim's hubID.");
    svDataMgr.UpdateSystemHubID(m_self->locationID(), 0);
    StructureSE::SetOffline();
}

bool IHubSE::CheckShieldReinforce()
{
    // Called from Damage.cpp when shield drops below 25%
    if (m_data.state == EVEPOS::StructureState::SheildReinforced
        || m_data.state == EVEPOS::StructureState::ArmorReinforced)
        return false; // already in reinforcement

    if (m_data.state == EVEPOS::StructureState::Online
        || m_data.state == EVEPOS::StructureState::Vulnerable) {
        SetReinforce(EVEPOS::ProcState::Reinforcing);
        _log(POS__MESSAGE, "IHub %s(%u): Entered Shield Reinforced (shield < 25%%).", GetName(), m_data.itemID);
        return true;
    }
    return false;
}

bool IHubSE::CheckReinforce()
{
    // Called from Damage.cpp when armor is depleted
    _log(POS__MESSAGE, "IHub %s(%u): CheckReinforce called (armor depleted), state=%i",
         GetName(), m_data.itemID, m_data.state);

    if (m_data.state == EVEPOS::StructureState::ArmorReinforced)
        return false; // already in final reinforcement, let it die

    if (m_data.state == EVEPOS::StructureState::SheildReinforced) {
        SetReinforce(EVEPOS::ProcState::ArmorReinforcing);
        _log(POS__MESSAGE, "IHub %s(%u): Entered Armor Reinforced (armor depleted).", GetName(), m_data.itemID);
        return true;
    }

    return false;
}
