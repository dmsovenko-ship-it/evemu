/**
 * @name Outpost.cpp
 *   Class for Outposts and Construction Platforms.
 *
 * @Author:           James
 * @date:   17 October 2021
 */


#include "eve-server.h"

#include "station/Outpost.h"

OutpostSE::OutpostSE(StationItemRef station, EVEServiceManager &services, SystemManager* system)
: StationSE(station, services, system)
{
}

void OutpostSE::SpawnStationService(Client* pClient, StationData stData, uint32 serviceType)
{
    /** @todo  this will require station service entities, not implemented yet */
}
