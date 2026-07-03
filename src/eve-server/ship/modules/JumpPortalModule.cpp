
 /**
  * @name JumpPortalModule.cpp
  *   Jump portal generator (titan bridge) module class
  * @date:   2026-07-03
  */

#include "ship/modules/JumpPortalModule.h"
#include "system/SystemManager.h"
#include "fleet/FleetService.h"
#include "pos/Tower.h"
#include "system/sov/SovereigntyDataMgr.h"

JumpPortalModule::JumpPortalModule(ModuleItemRef mRef, ShipItemRef sRef)
: ActiveModule(mRef, sRef),
pClient(nullptr),
pShipSE(nullptr),
m_beaconID(0),
m_targetSystemID(0),
m_shipVelocity(0.0f)
{
    if (!m_shipRef->HasPilot())
        return;

    pClient = m_shipRef->GetPilot();
}

void JumpPortalModule::Activate(uint16 effectID, uint32 targetID, int16 repeat)
{
    pShipSE = pClient->GetShipSE();
    m_beaconID = targetID;

    // Try to determine the target system from the beacon item
    InventoryItemRef beacon = sItemFactory.GetItemRefFromID(targetID);
    if (beacon) {
        m_targetSystemID = beacon->locationID();
    } else {
        m_targetSystemID = 0;
    }

    ActiveModule::Activate(effectID, targetID, repeat);

    if (m_Stop)
        return;

    _log(MODULE__DEBUG, "Jump portal generator activated by %s in %s, beaconID=%u",
         pClient->GetName(), m_sysMgr->GetName(), m_beaconID);

    // Freeze ship movement while portal is active
    m_shipVelocity = pShipSE->DestinyMgr()->GetMaxVelocity();
    pShipSE->DestinyMgr()->SetFrozen(true);

    // Send fleet notification about the active bridge
    SendOnJumpBeaconChange(true);
}

void JumpPortalModule::DeactivateCycle(bool abort)
{
    SendOnJumpBeaconChange(false);
    ActiveModule::DeactivateCycle(abort);

    pShipSE->GetSelf()->SetAttribute(AttrMaxVelocity, m_shipVelocity);
    pShipSE->DestinyMgr()->SetFrozen(false);
}

bool JumpPortalModule::CanActivate()
{
    if (m_beaconID == 0) {
        pClient->SendNotifyMsg("You must target a cyno field to create a jump portal.");
        return false;
    }

    // Must be in fleet to bridge
    if (!pClient->InFleet()) {
        pClient->SendNotifyMsg("You must be in a fleet to activate a jump portal.");
        return false;
    }

    if (pShipSE->SysBubble()->HasTower()) {
        TowerSE* ptSE = pShipSE->SysBubble()->GetTowerSE();
        if (ptSE->HasForceField())
            if (pShipSE->GetPosition().distance(ptSE->GetPosition()) < ptSE->GetSOI())
                throw UserError("NoCynoInPOSShields");
    }

    SovereigntyData sovData = svDataMgr.GetSovereigntyData(pClient->GetLocationID());
    if (sovData.jammerID != 0) {
        pClient->SendNotifyMsg("This system is currently being jammed.");
        return false;
    }

    if (!sConfig.world.highSecCyno) {
        if (pClient->SystemMgr()->GetSecValue() >= 0.5f) {
            pClient->SendNotifyMsg("This module may not be used in high security space.");
            return false;
        }
    }

    return ActiveModule::CanActivate();
}

void JumpPortalModule::SendOnJumpBeaconChange(bool active/*false*/) {
    _log(MODULE__DEBUG, "JumpPortalModule: Sending OnJumpBeaconChange (active = %s)", active ? "true" : "false");

    uint32 fieldID(pShipSE->GetID());

    PyTuple* data = new PyTuple(4);
        data->SetItem(0, new PyInt(pClient->GetCharacterID()));
        data->SetItem(1, new PyInt(m_sysMgr->GetID()));
        data->SetItem(2, new PyInt(fieldID));
        data->SetItem(3, new PyBool(active));

    std::vector<Client*> fleetClients;
    fleetClients = sFltSvc.GetFleetClients(pClient->GetFleetID());
    for (auto cur : fleetClients)
        if (cur != nullptr) {
            cur->SendNotification("OnJumpBeaconChange", "clientID", &data);
            _log(MODULE__DEBUG, "OnJumpBeaconChange sent to %s (%u)", cur->GetName(), cur->GetCharID());
        }
}
