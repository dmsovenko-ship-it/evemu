/**
 * @name Outpost.h
 *   Class for Outposts.
 *
 * @Author:           James
 * @date:   17 October 2021
 */

#ifndef EVEMU_POS_OUTPOST_H_
#define EVEMU_POS_OUTPOST_H_

#include "DataClasses.h"
#include "pos/Structure.h"
#include "station/Station.h"

class PlatformSE
: public StructureSE
{
public:
    PlatformSE(StructureItemRef structure, EVEServiceManager &services, SystemManager *system, const FactionData &fData)
    : StructureSE(structure, services, system, fData)   { /* do nothing here */ }

    virtual ~PlatformSE()                               { /* do nothing here */ }

    virtual PlatformSE*         GetPlatformSE()         { return this; }

    virtual bool                IsPlatformSE()          { return true; }
    virtual bool                isGlobal()              { return true; }
    virtual bool                IsOperSE()              { return true; }
};

class OutpostSE
: public StationSE
{
public:
    OutpostSE(StationItemRef station, EVEServiceManager &services, SystemManager* system);
    virtual ~OutpostSE()                                { /* Do nothing here */ }

    virtual OutpostSE* GetOutpostSE()                   { return this; }
    virtual bool IsOutpostSE()                          { return true; }

    // Capture mechanics
    bool IsConquerable()                                { return m_conquerable; }
    bool CheckReinforce();
    void Capture(Damage& damage);
    int8 GetReinforceState()        { return m_reinforceState; }
    virtual void Killed(Damage& damage);

private:
    bool m_conquerable;
    int8 m_reinforceState;   // 0=Online, 1=ShieldReinforced, 2=ArmorReinforced
    int64 m_reinforceEnd;    // Win32 time when reinforcement ends
};

#endif  // EVEMU_POS_OUTPOST_H_
