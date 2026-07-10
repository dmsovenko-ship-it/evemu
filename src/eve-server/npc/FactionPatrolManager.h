#ifndef EVEMU_SYSTEM_FACTIONPATROLMANAGER_H_
#define EVEMU_SYSTEM_FACTIONPATROLMANAGER_H_

#include "eve-common.h"
#include <map>

class SystemManager;

class FactionPatrolManager
{
public:
    static void SpawnPatrols(SystemManager* sysMgr);
};

#endif
