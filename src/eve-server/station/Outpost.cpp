/**
 * @name Outpost.cpp
 *   Class for Outposts and Construction Platforms.
 *
 * @Author:           James
 * @date:   17 October 2021
 */


#include "eve-server.h"

#include "EntityList.h"
#include "EVEServerConfig.h"
#include "station/Outpost.h"
#include "system/Damage.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

OutpostSE::OutpostSE(StationItemRef station, EVEServiceManager &services, SystemManager* system)
: StationSE(station, services, system)
{
}

void OutpostSE::SpawnStationService(Client* pClient, StationData stData, uint32 serviceType)
{
    // Base class handles service spawning
    StationSE::SpawnStationService(pClient, stData, serviceType);
}

bool OutpostSE::CheckReinforce()
{
    _log(POS__MESSAGE, "Outpost %s(%u): CheckReinforce called, state=%i",
         GetName(), m_data.itemID, m_data.state);

    // Only conquerable outposts can be captured
    if (!IsConquerable())
        return false;

    if (m_data.state == EVEPOS::StructureState::ArmorReinforced)
        return false; // final reinforcement — let it be destroyed for capture

    if (m_data.state == EVEPOS::StructureState::SheildReinforced) {
        SetReinforce(EVEPOS::ProcState::ArmorReinforcing);
        _log(POS__MESSAGE, "Outpost %s(%u): Entered Armor Reinforced.", GetName(), m_data.itemID);
        return true;
    }

    if (m_data.state == EVEPOS::StructureState::Online
        || m_data.state == EVEPOS::StructureState::Vulnerable) {
        SetReinforce(EVEPOS::ProcState::Reinforcing);
        _log(POS__MESSAGE, "Outpost %s(%u): Entered Shield Reinforced.", GetName(), m_data.itemID);
        return true;
    }

    return false;
}

void OutpostSE::SetReinforce(EVEPOS::ProcState pState)
{
    int64 reinforceTime = 48 * EvE::Time::Hour * 1000; // 48h
    _log(POS__MESSAGE, "Outpost %s(%u): Entering reinforcement (state %i).", GetName(), m_data.itemID, pState);
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
