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
#include "../../eve-common/EVE_POS.h"

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

    virtual void SpawnStationService(Client* pClient, StationData stData, uint32 serviceType);

    // Dominion capture mechanics
    bool IsConquerable()                                { return m_conquerable; }
    void SetIsConquerable(bool c)                       { m_conquerable = c; }
    bool CheckReinforce();
    void SetReinforce(EVEPOS::ProcState pState);
    void Capture(Damage& damage);
    virtual void Killed(Damage& damage);

    uint32 GetCorporationID()                           { return m_corpID; }

private:
    bool m_conquerable = false;
};

#endif  // EVEMU_POS_OUTPOST_H_
