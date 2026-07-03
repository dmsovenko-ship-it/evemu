
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
    virtual uint32 DoCycle();
    virtual bool CanActivate();

    // Accessors for bridge commands
    bool IsPortalActive() { return (m_portalSE != nullptr); }
    SystemEntity* GetPortalSE() { return m_portalSE; }
    uint32 GetBeaconID() { return m_beaconID; }
    uint32 GetBridgeTargetID() { return m_bridgeTargetID; }
    void SetBridgeTargetID(uint32 shipID) { m_bridgeTargetID = shipID; }

protected:
    Client* pClient;
    ShipSE* pShipSE;
    SystemEntity* m_portalSE;

    bool m_firstRun;
    uint32 m_beaconID;          // target cyno beacon for the bridge
    uint32 m_bridgeTargetID;    // target fleet member ship for bridge-to-member
    uint32 m_targetSystemID;    // target solar system
    float m_shipVelocity;

    void CreatePortal();
    void RemovePortal();
    void SendOnJumpBeaconChange(bool active=false);
};

#endif  //_EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_
