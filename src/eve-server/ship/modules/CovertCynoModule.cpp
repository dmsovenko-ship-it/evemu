
 /**
  * @name CovertCynoModule.cpp
  *   Covert Cynosural field generator module class
  * @date:   2026-07-03
  */

#include "ship/modules/CovertCynoModule.h"
#include "system/SystemManager.h"

CovertCynoModule::CovertCynoModule(ModuleItemRef mRef, ShipItemRef sRef)
: CynoModule(mRef, sRef)
{
}

bool CovertCynoModule::CanActivate()
{
    // Covert cyno does NOT require fleet membership
    // Check POS shields
    if (pShipSE->SysBubble()->HasTower()) {
        TowerSE* ptSE = pShipSE->SysBubble()->GetTowerSE();
        if (ptSE->HasForceField())
            if (pShipSE->GetPosition().distance(ptSE->GetPosition()) < ptSE->GetSOI())
                throw UserError("NoCynoInPOSShields");
    }

    /** @todo check for active cyno jammer */

    SovereigntyData sovData = svDataMgr.GetSovereigntyData(pClient->GetLocationID());
    if (sovData.jammerID != 0) {
        pClient->SendNotifyMsg("This system is currently being jammed.");
        return false;
    }

    // Covert cyno can be used in high-sec (it's covert)
    // No high-sec restriction

    // all specific checks pass.  run generic checks in base class
    return ActiveModule::CanActivate();
}

void CovertCynoModule::CreateCyno()
{
    if (cSE != nullptr)
        return;

    ItemData cData(EVEDB::invTypes::CovertCynosuralFieldI, pClient->GetCharacterID(), m_sysMgr->GetID(), flagNone);
    InventoryItemRef cRef = sItemFactory.SpawnItem(cData);

    _log(MODULE__DEBUG, "Creating Covert Cynosural field");

    cSE = new ItemSystemEntity(cRef, pClient->services(), m_sysMgr);
    GPoint location(pShipSE->GetPosition());
    location.MakeRandomPointOnSphere(1500.0f + cRef->type().radius());
    cSE->SetPosition(location);
    cRef->SaveItem();
    m_sysMgr->AddEntity(cSE);

    SendOnJumpBeaconChange(true);
}
