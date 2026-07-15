
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
: StructureSE(structure, services, system, fData)
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
    // Called when IHub HP drops below threshold
    // Enters the appropriate reinforcement state
    int64 reinforceTime = 48 * EvE::Time::Hour * 1000; // 48h default
    _log(POS__MESSAGE, "IHub %s(%u): Entering reinforcement (state %i).", GetName(), m_data.itemID, pState);
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
    m_self->SetFlag(flagStructureInactive);
    SetTimer(reinforceTime);
    m_db.UpdateBaseData(m_data);
    SendSlimUpdate();

    // Notify corporation about reinforcement
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

void IHubSE::Reinforce()
{
    // IHub has been attacked and needs to enter reinforcement
    // Check current HP percentages to determine reinforcement level
    double shieldPct = m_self->GetAttribute(AttrShieldCharge).get_float() /
                       std::max(m_self->GetAttribute(AttrShieldCapacity).get_float(), 1.0f);
    double armorPct = m_self->GetAttribute(AttrArmorDamage).get_float() /
                      std::max(m_self->GetAttribute(AttrArmorHP).get_float(), 1.0f);

    if (armorPct > 0.5f) {
        // Armor below 50% → second reinforcement or destruction
        if (m_data.state == EVEPOS::StructureState::ArmorReinforced) {
            // Already in armor reinforcement, let it be destroyed
            return;
        }
        SetReinforce(EVEPOS::ProcState::ArmorReinforcing);
    } else if (shieldPct < 0.25f) {
        // Shield below 25% → first reinforcement
        if (m_data.state == EVEPOS::StructureState::Vulnerable
            || m_data.state == EVEPOS::StructureState::Online) {
            SetReinforce(EVEPOS::ProcState::Reinforcing);
        }
    }
}
