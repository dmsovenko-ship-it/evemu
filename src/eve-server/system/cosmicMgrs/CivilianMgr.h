
 /**
  * @name CivilianMgr.h
  *     Civilian (non-combatant NPC) management system for EVEmu
  *
  * @Author:        Allan
  * @date:          12 Feb 2017
  *
  */



#ifndef EVEMU_SYSTEM_CIVILIANMGR_H_
#define EVEMU_SYSTEM_CIVILIANMGR_H_


#include "ServiceDB.h"
#include "utils/Singleton.h"
#include "utils/timer.h"

#include <map>

/* this class will control all aspects of
 * non-combatant civilians
 */

class EVEServiceManager;
class SystemManager;
struct ConvoyGroup;

class CivilianMgr
: public Singleton<CivilianMgr>
{
public:
    CivilianMgr();
    ~CivilianMgr();

    void Initialize();
    void Process();

private:
    void SpawnSystemCivilians(SystemManager* sysMgr);
    void RemoveSystemCivilians(uint32 sysID);

    ServiceDB* m_db;
    Timer* m_processTimer;
    bool m_initalized;
    std::map<uint32, ConvoyGroup*> m_systemCivs;

};

//Singleton
#define sCivMgr \
( CivilianMgr::get() )


#endif  // EVEMU_SYSTEM_CIVILIANMGR_H_
