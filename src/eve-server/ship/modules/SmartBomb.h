#ifndef SMARTBOMB_H
#define SMARTBOMB_H

#include "ship/modules/ActiveModule.h"

class SmartBomb : public ActiveModule {
public:
    SmartBomb(ModuleItemRef mRef, ShipItemRef sRef);
    virtual ~SmartBomb() = default;

    virtual uint32      DoCycle();
};

#endif
