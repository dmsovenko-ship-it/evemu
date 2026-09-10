
/**
 * @name Battery.h
 *   Class for POS Battery Modules.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */


#ifndef EVEMU_POS_BATTERY_H_
#define EVEMU_POS_BATTERY_H_


#include "pos/Structure.h"


class POS_AI;

class BatterySE
: public StructureSE
{
public:
    BatterySE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& data);
    virtual ~BatterySE();

    /* class type pointer querys. */
    virtual BatterySE*          GetBatterySE()          { return this; }

    /* class type tests. */
    virtual bool                IsBatterySE()           { return true; }

    /* SystemEntity interface */
    virtual void                Process();
    virtual void                TargetLost(uint32 entityID);

    /* virtual functions default to base class and overridden as needed */
    virtual void Init();

private:
    POS_AI* m_ai;
};

#endif  // EVEMU_POS_BATTERY_H_
