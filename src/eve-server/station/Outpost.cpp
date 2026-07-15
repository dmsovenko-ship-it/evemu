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
: StationSE(station, services, system),
  m_conquerable(false)
{
}

void OutpostSE::SpawnStationService(Client* pClient, StationData stData, uint32 serviceType)
{
    StationSE::SpawnStationService(pClient, stData, serviceType);
}

bool OutpostSE::CheckReinforce()
{
    _log(POS__MESSAGE, "Outpost %s(%u): CheckReinforce called, state=%i",
         GetName(), m_data.itemID, m_data.state);

    if (!IsConquerable())
        return false;

    if (m_data.state == EVEPOS::StructureState::ArmorReinforced)
        return false; // final reinforcement → capture

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

void OutpostSE::Killed(Damage& damage)
{
    // Check if this is a capture event (armor reinforced → final destruction)
    if (IsConquerable() && m_data.state == EVEPOS::StructureState::ArmorReinforced) {
        Capture(damage);
        return; // station captured, not destroyed
    }
    // Non-conquerable or non-reinforced: normal destruction
    StationSE::Killed(damage);
}

void OutpostSE::Capture(Damage& damage)
{
    _log(POS__MESSAGE, "Outpost %s(%u): Captured!", GetName(), m_data.itemID);

    // Determine attacker
    uint32 newCorpID = 0;
    uint32 newAllianceID = 0;
    SystemEntity* killer = damage.srcSE;

    if (killer != nullptr) {
        if (killer->HasPilot()) {
            Client* pClient = killer->GetPilot();
            newCorpID = pClient->GetCorporationID();
            newAllianceID = pClient->GetAllianceID();
        } else if (killer->IsDroneSE()) {
            Client* pClient = sEntityList.FindClientByCharID(killer->GetSelf()->ownerID());
            if (pClient != nullptr) {
                newCorpID = pClient->GetCorporationID();
                newAllianceID = pClient->GetAllianceID();
            }
        }
    }

    if (newCorpID == 0) {
        _log(POS__ERROR, "Outpost %s(%u): Capture failed — cannot determine attacker.", GetName(), m_data.itemID);
        return;
    }

    // Transfer station ownership
    uint32 stationID = m_self->itemID();

    // Update DB
    m_db.UpdateBaseData(m_data);
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE staStations SET corporationID = %u WHERE stationID = %u",
        newCorpID, stationID);

    // Update sovereignty claim with new owner
    SovereigntyData sovData = svDataMgr.GetSovereigntyData(m_system->GetID());
    if (sovData.claimID > 0) {
        // Update existing claim with new corp/alliance
        svDataMgr.RemoveSovClaim(m_system->GetID());
        sovData.corporationID = newCorpID;
        sovData.allianceID = newAllianceID;
        svDataMgr.AddSovClaim(sovData);
    }

    // Restore station to full health and online state
    m_self->SetAttribute(AttrShieldCharge, m_self->GetAttribute(AttrShieldCapacity));
    m_self->SetAttribute(AttrArmorDamage, EvilZero);
    m_self->SetAttribute(AttrDamage, EvilZero);
    m_self->SetFlag(flagStructureActive);
    m_data.state = EVEPOS::StructureState::Online;
    m_procState = EVEPOS::ProcState::Online;
    m_db.UpdateBaseData(m_data);
    SendSlimUpdate();

    _log(POS__MESSAGE, "Outpost %s(%u): Captured by corp %u (ally %u). Station restored.",
         GetName(), m_data.itemID, newCorpID, newAllianceID);

    // Notify all clients about sovereignty change
    PyDict* args = new PyDict();
    args->SetItemString("contested", new PyInt(sovData.contested));
    args->SetItemString("corporationID", new PyInt(newCorpID));
    args->SetItemString("allianceID", new PyInt(newAllianceID));
    args->SetItemString("solarSystemID", new PyInt(m_system->GetID()));

    PyTuple* data = new PyTuple(2);
        data->SetItem(0, new PyInt(m_system->GetID()));
        data->SetItem(1, new PyObject("util.KeyVal", args));

    std::vector<Client*> clients;
    sEntityList.GetClients(clients);
    for (auto cur : clients) {
        if (cur != nullptr)
            cur->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }
}

void OutpostSE::SetReinforce(int8 pState)
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
