/**
 * @name Outpost.cpp
 *   Class for Outposts and Construction Platforms.
 *
 * @Author:           James
 * @date:   17 October 2021
 */


#include "eve-server.h"

#include "Client.h"
#include "EntityList.h"
#include "EVE_Mail.h"
#include "station/Outpost.h"
#include "system/Damage.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

OutpostSE::OutpostSE(StationItemRef station, EVEServiceManager &services, SystemManager* system)
: StationSE(station, services, system),
  m_conquerable(true),
  m_reinforceState(0),
  m_reinforceEnd(0)
{
}

void OutpostSE::SpawnStationService(Client* pClient, StationData stData, uint32 serviceType)
{
    // Station service entities will be implemented later
    _log(POS__DEBUG, "Outpost::SpawnStationService(%u) called for %s(%u) — stub.", serviceType, GetName(), m_self->itemID());
}

bool OutpostSE::CheckReinforce()
{
    if (!m_conquerable)
        return false;

    // Check if current reinforcement timer has expired
    if (m_reinforceState > 0 && GetFileTimeNow() >= m_reinforceEnd) {
        // Timer expired — advance state
        if (m_reinforceState == 1) {
            // Shield reinforced → armor reinforced
            m_reinforceState = 2;
            m_reinforceEnd = GetFileTimeNow() + 24 * EvE::Time::Hour;
            _log(POS__MESSAGE, "Outpost %s(%u): Shield reinforcement expired, entering Armor Reinforced.", GetName(), m_self->itemID());
            return true;
        } else if (m_reinforceState == 2) {
            // Armor reinforced → vulnerable (can be captured now)
            m_reinforceState = 0;
            m_reinforceEnd = 0;
            _log(POS__MESSAGE, "Outpost %s(%u): Armor reinforcement expired, now vulnerable.", GetName(), m_self->itemID());
            return true;
        }
    }

    // Called when the outpost is about to be destroyed — check if it should reinforce instead
    if (m_reinforceState == 0) {
        // Not in reinforcement → enter shield reinforcement
        m_reinforceState = 1;
        m_reinforceEnd = GetFileTimeNow() + 24 * EvE::Time::Hour;
        _log(POS__MESSAGE, "Outpost %s(%u): Entering Shield Reinforced (24h).", GetName(), m_self->itemID());
        return true;
    }

    if (m_reinforceState == 1) {
        // Already in shield reinforced, timer hasn't expired yet — ignore damage
        return true;
    }

    if (m_reinforceState == 2) {
        // Armor reinforced and timer expired → capture instead of destroy
        return false; // Let it be destroyed — Killed() will handle capture
    }

    return false;
}

void OutpostSE::Killed(Damage& damage)
{
    if (m_conquerable && m_reinforceState == 2 && GetFileTimeNow() >= m_reinforceEnd) {
        // Armor reinforcement expired → capture instead of destroy
        Capture(damage);
        return;
    }
    // Normal kill
    SystemEntity::Killed(damage);
}

void OutpostSE::Capture(Damage& damage)
{
    _log(POS__MESSAGE, "Outpost %s(%u): Captured!", GetName(), m_self->itemID());

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
        _log(POS__ERROR, "Outpost %s(%u): Capture failed — cannot determine attacker.", GetName(), m_self->itemID());
        return;
    }

    // Transfer ownership in DB
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE staStations SET corporationID = %u WHERE stationID = %u",
        newCorpID, m_self->itemID());

    // Restore station to full health
    m_self->SetAttribute(AttrShieldCharge, m_self->GetAttribute(AttrShieldCapacity));
    m_self->SetAttribute(AttrArmorDamage, EvilZero);
    m_self->SetAttribute(AttrDamage, EvilZero);

    m_reinforceState = 0;
    m_reinforceEnd = 0;

    // Notify
    PyTuple* data = new PyTuple(2);
        data->SetItem(0, new PyInt(m_system->GetID()));
        data->SetItem(1, PyStatic.NewNone());

    std::vector<Client*> clients;
    sEntityList.GetClients(clients);
    for (auto* c : clients) {
        if (c != nullptr)
            c->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }

    _log(POS__MESSAGE, "Outpost %s(%u): Captured by corp %u (ally %u).", GetName(), m_self->itemID(), newCorpID, newAllianceID);
}
