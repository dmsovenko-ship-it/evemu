
/**
 * @name TCU.cpp
 *   Class for Territorial Claim Units.
 *
 * @Author:         James
 * @date:   8 April 2021
 */


#include "Client.h"
#include "EntityList.h"
#include "EVEServerConfig.h"
#include "planet/Planet.h"
#include "pos/sovStructures/TCU.h"
#include "packets/Sovereignty.h"
#include "system/Container.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"
#include "system/sov/SovereigntyDataMgr.h"

TCUSE::TCUSE(StructureItemRef structure, EVEServiceManager&services, SystemManager *system, const FactionData &fData)
    : StructureSE(structure, services, system, fData),
      m_claimTime(0)
{
}

void TCUSE::Init()
{
    _log(SE__TRACE, "TCUSE %s(%u) is being initialized", m_self->name(), m_self->itemID());
    StructureSE::Init();

    if (m_bubble == nullptr)
        assert(0);
    m_bubble->SetTCUSE(this);

    m_self->SetAttribute(AttrIsGlobal, EvilOne, false);
}

void TCUSE::SetOnline()
{
    _log(SOV__DEBUG, "Onlining TCU... Starting 8-hour claim timer.");
    StructureSE::SetOnline();

    // Start 8-hour claim timer
    m_claimTime = GetFileTimeNow() + 8 * EvE::Time::Hour;
    _log(SOV__MESSAGE, "TCU %s(%u): Claim will finalize at %lli", GetName(), m_data.itemID, m_claimTime);

    // Send notification that claim process has started
    PyTuple* data = new PyTuple(2);
        data->SetItem(0, new PyInt(m_system->GetID()));
        data->SetItem(1, PyStatic.NewNone());

    // Notify all clients that sovereignty is pending
    std::vector<Client*> list;
    sEntityList.GetClients(list);
    for (auto cur : list)
    {
        if (cur != nullptr)
            cur->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }
}

void TCUSE::SetOffline()
{
    _log(SOV__DEBUG, "Offlining TCU... Removing claim if active.");
    if (m_claimTime > 0) {
        // Claim was pending — cancelled
        m_claimTime = 0;
        _log(SOV__MESSAGE, "TCU %s(%u): Pending claim cancelled (offlined).", GetName(), m_data.itemID);
    }
    svDataMgr.RemoveSovClaim(m_system->GetID());

    PyTuple* data = new PyTuple(2);
        data->SetItem(0, new PyInt(m_system->GetID()));
        data->SetItem(1, PyStatic.NewNone());

    std::vector<Client*> list;
    sEntityList.GetClients(list);
    for (auto cur : list)
    {
        if (cur != nullptr)
            cur->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }

    StructureSE::SetOffline();
}

void TCUSE::Process()
{
    StructureSE::Process();

    // Check 8-hour claim timer
    if (m_claimTime > 0 && GetFileTimeNow() >= m_claimTime) {
        FinalizeClaim();
    }
}

void TCUSE::Killed(Damage& damage)
{
    // Cancel pending claim if TCU is destroyed during claiming period
    if (m_claimTime > 0) {
        _log(SOV__MESSAGE, "TCU %s(%u): Pending claim cancelled (destroyed during claiming period).", GetName(), m_data.itemID);
        m_claimTime = 0;
    }
    StructureSE::Killed(damage);
}

void TCUSE::FinalizeClaim()
{
    if (m_claimTime == 0)
        return;
    m_claimTime = 0;

    _log(SOV__MESSAGE, "TCU %s(%u): 8-hour claim timer expired — creating sovereignty claim.", GetName(), m_data.itemID);

    SovereigntyData sovData = SovereigntyData();
        sovData.solarSystemID = m_system->GetID();
        sovData.regionID = m_system->GetRegionID();
        sovData.constellationID = m_system->GetConstellationID();
        sovData.corporationID = m_corpID;
        sovData.allianceID = m_allyID;
        sovData.claimStructureID = m_data.itemID;
        sovData.claimTime = GetFileTimeNow();
    svDataMgr.AddSovClaim(sovData);

    PyDict *args = new PyDict;
    args->SetItemString("contested", new PyInt(sovData.contested));
    args->SetItemString("corporationID", new PyInt(sovData.corporationID));
    args->SetItemString("claimTime", new PyLong(sovData.claimTime));
    args->SetItemString("claimStructureID", new PyInt(sovData.claimStructureID));
    args->SetItemString("hubID", new PyInt(sovData.hubID));
    args->SetItemString("allianceID", new PyInt(sovData.allianceID));
    args->SetItemString("solarSystemID", new PyInt(sovData.solarSystemID));

    PyTuple* data = new PyTuple(2);
        data->SetItem(0, new PyInt(sovData.solarSystemID));
        data->SetItem(1, new PyObject("util.KeyVal", args));

    std::vector<Client*> list;
    sEntityList.GetClients(list);
    for (auto cur : list)
    {
        if (cur != nullptr)
            cur->SendNotification("ProcessSovStatusChanged", "clientID", &data);
    }
}
