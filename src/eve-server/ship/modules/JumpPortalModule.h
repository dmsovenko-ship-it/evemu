
 /**
  * @name JumpPortalModule.h
  *   Jump portal generator (titan bridge) module class
  * @date:   2026-07-03
  */

#ifndef _EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_
#define _EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_

#include "ship/modules/ActiveModule.h"
#include "system/SystemEntity.h"
#include "Client.h"

class JumpPortalModule : public ActiveModule
{
public:
    JumpPortalModule(ModuleItemRef mRef, ShipItemRef sRef);
    virtual ~JumpPortalModule() { /* do nothing here */ }

    virtual JumpPortalModule* GetJumpPortalModule() { return this; }

    /* ActiveModule overrides */
    virtual void Activate(uint16 effectID, uint32 targetID=0, int16 repeat=0);
    virtual void DeactivateCycle(bool abort=false);
    virtual bool CanActivate();

    // Accessors for bridge commands
    bool IsPortalActive() { return IsActive(); }
    uint32 GetBeaconID() { return m_beaconID; }

protected:
    Client* pClient;
    ShipSE* pShipSE;

    uint32 m_beaconID;          // target cyno beacon for the bridge
    uint32 m_targetSystemID;    // target solar system
    float m_shipVelocity;

    void SendOnJumpBeaconChange(bool active=false);
};

#endif  //_EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_
