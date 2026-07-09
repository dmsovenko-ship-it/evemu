
 /**
  * @name CivilianMgr.h
  *     Civilian (non-combatant NPC) management system for EVEmu
  *
  * @Author:        Allan
  * @date:          12 Feb 2017
  */

#ifndef EVEMU_SYSTEM_CIVILIANMGR_H_
#define EVEMU_SYSTEM_CIVILIANMGR_H_

#include "ServiceDB.h"
#include "utils/Singleton.h"
#include "utils/timer.h"

#include <map>

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
    void TransferCrossSystem(ConvoyGroup* group);   // called by ConvoyAI on gate jump

private:
    struct StargateLink {
        uint32 sourceGateID;
        uint32 destGateID;
        uint32 destSystemID;
    };
    std::vector<StargateLink> GetStargateLinks(uint32 systemID);
    void SpawnSystemCivilians(SystemManager* sysMgr);
    void RemoveSystemCivilians(uint32 sysID);
    void RemoveConvoy(ConvoyGroup* group);
    void TransferCrossSystem(ConvoyGroup* group);
    void ResumeCrossSystem(ConvoyGroup* group);

    uint8 GetFactionForSystem(uint32 systemID);

    ServiceDB* m_db;
    Timer* m_processTimer;
    bool m_initalized;
    std::map<uint32, ConvoyGroup*> m_systemCivs;
    std::vector<ConvoyGroup*> m_transitConvoys;     // convoys in transit between systems
};

//Singleton
#define sCivMgr \
( CivilianMgr::get() )

#endif  // EVEMU_SYSTEM_CIVILIANMGR_H_