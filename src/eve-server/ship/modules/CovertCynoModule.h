
 /**
  * @name CovertCynoModule.h
  *   Covert Cynosural field generator module class
  * @date:   2026-07-03
  */

#ifndef _EVE_SHIP_MODULES_COVERT_CYNO_MODULE_H_
#define _EVE_SHIP_MODULES_COVERT_CYNO_MODULE_H_

#include "ship/modules/CynoModule.h"

class CovertCynoModule : public CynoModule
{
public:
    CovertCynoModule(ModuleItemRef mRef, ShipItemRef sRef);
    virtual ~CovertCynoModule() { /* do nothing here */ }

    virtual CovertCynoModule* GetCovertCynoModule() { return this; }

    /* ActiveModule overrides */
    virtual bool CanActivate();

protected:
    virtual void CreateCyno();
};

#endif  //_EVE_SHIP_MODULES_COVERT_CYNO_MODULE_H_
