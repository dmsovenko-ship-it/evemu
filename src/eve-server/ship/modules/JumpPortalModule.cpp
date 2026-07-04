
 /**
  * @name JumpPortalModule.cpp
  *   Jump portal generator (titan bridge) module class
  * @date:   2026-07-03
  */

#include "ship/modules/JumpPortalModule.h"
#include "system/SystemManager.h"
#include "fleet/FleetService.h"
#include "pos/Tower.h"
#include "pos/Structure.h"
#include "system/sov/SovereigntyDataMgr.h"

JumpPortalModule::JumpPortalModule(ModuleItemRef mRef, ShipItemRef sRef)
: ActiveModule(mRef, sRef),
pClient(nullptr),
pShipSE(nullptr),
m_portalSE(nullptr),
m_firstRun(true),
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

    m_firstRun = true;
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
    RemovePortal();
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
        SystemEntity* jammerSE = pShipSE->SystemMgr()->GetSE(sovData.jammerID);
        StructureSE* jammerStruct = dynamic_cast<StructureSE*>(jammerSE);
        if (jammerStruct != nullptr and jammerStruct->IsJammerSE())
            return false;
    }

    // Covert Jump Portal Generator (fitted only on Black Ops, group 898) can be used anywhere
    bool isCovert = m_modRef->HasAttribute(AttrCanFitShipGroup1)
                    and (m_modRef->GetAttribute(AttrCanFitShipGroup1).get_uint32() == EVEDB::invGroups::BlackOps);
    if (!isCovert and !sConfig.world.highSecCyno) {
        if (pClient->SystemMgr()->GetSecValue() >= 0.5f) {
            pClient->SendNotifyMsg("This module may not be used in high security space.");
            return false;
        }
    }

    return ActiveModule::CanActivate();
}

uint32 JumpPortalModule::DoCycle()
{
    uint32 retVal(0);
    if (retVal = ActiveModule::DoCycle())
        if (m_firstRun) {
            m_firstRun = false;
            CreatePortal();
        }

    return retVal;
}

void JumpPortalModule::CreatePortal()
{
    if (m_portalSE != nullptr)
        return;

    // Covert Jump Portal creates a covert field (only Black Ops can use it)
    bool isCovert = m_modRef->HasAttribute(AttrCanFitShipGroup1)
                    and (m_modRef->GetAttribute(AttrCanFitShipGroup1).get_uint32() == EVEDB::invGroups::BlackOps);
    uint32 fieldType = isCovert ? EVEDB::invTypes::CovertCynosuralFieldI : EVEDB::invTypes::CynosuralFieldI;

    ItemData pData(fieldType, pClient->GetCharacterID(), m_sysMgr->GetID(), flagNone);
    InventoryItemRef pRef = sItemFactory.SpawnItem(pData);

    _log(MODULE__DEBUG, "Creating jump portal");

    m_portalSE = new ItemSystemEntity(pRef, pClient->services(), m_sysMgr);
    GPoint location(pShipSE->GetPosition());
    location.MakeRandomPointOnSphere(2000.0f + pRef->type().radius());
    m_portalSE->SetPosition(location);
    pRef->SaveItem();
    m_sysMgr->AddEntity(m_portalSE);
}

void JumpPortalModule::RemovePortal()
{
    if (m_portalSE != nullptr) {
        m_portalSE->Delete();
        SafeDelete(m_portalSE);
    }
}

void JumpPortalModule::OnModuleOnline() {
    // Create portal automatically when module comes online
    if (m_portalSE == nullptr and m_shipRef->HasPilot() and pClient->GetShipSE() != nullptr) {
        pShipSE = pClient->GetShipSE();
        CreatePortal();
        SendOnJumpBeaconChange(true);
    }
}

void JumpPortalModule::OnModuleOffline() {
    // Remove portal when module goes offline
    RemovePortal();
    SendOnJumpBeaconChange(false);
}

void JumpPortalModule::SendOnJumpBeaconChange(bool active/*false*/) {
    _log(MODULE__DEBUG, "JumpPortalModule: Sending OnJumpBeaconChange (active = %s)", active ? "true" : "false");

    uint32 fieldID(0);
    if (m_portalSE != nullptr)
        fieldID = m_portalSE->GetID();
    else
        fieldID = pShipSE->GetID();

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
