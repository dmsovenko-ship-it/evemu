
 /**
  * @name SpawnMgr.h
  *     NPC Spawn management system for EVEmu
  *
  * @Author:    Allan
  * @date:      15Jul15
  *
  */


#ifndef _EVE_NPC_SPAWNMGR_H__
#define _EVE_NPC_SPAWNMGR_H__

#include <unordered_map>
#include "../../../eve-common/EVE_Spawn.h"
#include "system/cosmicMgrs/ManagerDB.h"

class NPC;
class EVEServiceManager;
class SystemManager;
class SystemBubble;
class DungeonMgr;

class SpawnMgr
{
public:
    SpawnMgr(SystemManager* mgr, EVEServiceManager& svc);
    ~SpawnMgr()                                         { /* nothing do to yet */ }

    bool Init();

    void Process();
    void SetDungMgr(DungeonMgr* pDmgr)                  { m_dungMgr = pDmgr; }
    // not working... warp a spawned npc group from one location to another (change bubbles)
    void WarpOutSpawn(NPC* pNPC, SystemBubble* pBubble);
    // update SpawnMgr on npcs new location (change bubbles)
    void MoveSpawn(NPC* pNPC, SystemBubble* pBubble);

    std::string GetSpawnClassName(int8 sClass);
    std::string GetSpawnGroupName(int8 sGroup);

    bool DoSpawnForBubble(SystemBubble* pBubble);
    void DoSpawnForAnomaly(SystemBubble* pBubble, GPoint pos, uint8 level, uint16 typeID, bool isIncursion = false);
    // DungeonMgr registers each incursion site bubble (dungeonID, wave #,
    // pocket center); the wave chain itself runs out of SpawnKilled.
    void RegisterIncursionWave(uint32 bubbleID, uint32 dungeonID, uint8 waveNum, const GPoint& pocket) {
        IncursionWave w;
        w.dungeonID = dungeonID;
        w.waveNum   = waveNum;
        w.pocket    = pocket;
        m_incursionAlive.erase(bubbleID);
        m_incursionWave[bubbleID] = w;
    }
    // Spawns wave N of an incursion at the new pocket + places the 17831 gate
    // that leads there (from the cleared pocket in wave N-1).
    void SpawnIncursionWave(uint32 dungeonID, uint8 waveNum, const GPoint& toPocket);
    void DoSpawnForMission(SystemBubble* pBubble, uint32 regionID);
    void DoSpawnForIncursion(SystemBubble* pBubble, uint32 regionID, uint8 sceneType = 3, uint32 incursionID = 0);
    void DoSpawnMothership(SystemBubble* pBubble, uint32 incursionID);

    // primitive test for chained spawns
    bool IsChaining(uint16 bubbleID);
    // this will be used for all spawn types
    void SpawnKilled(SystemBubble* pBubble, uint32 itemID);    // this DOES NOT remove entity from system or bubble.  user must do this BEFORE calling.

    void StartRatTimer();
    void StopRatTimer()                                 { m_ratTimer.Disable(); }
    void StartRatGroupTimer();
    void StopRatGroupTimer()                            { m_ratGroupTimer.Disable(); }

    bool IsInitialized()                                { return m_initalized; }
    bool IsRatTimerStarted()                            { return m_ratTimer.Enabled(); }


protected:
    bool FindSpawnForBubble(uint16 bubbleID);
    bool PrepSpawn(SystemBubble* pBubble, uint8 sClass = Spawn::Class::None, uint8 level = 0);
    void MakeSpawn(SystemBubble* pBubble, uint32 factionID, uint8 sClass, uint8 level, bool anomaly=false);
    void ReSpawn(SystemBubble* pBubble, SpawnEntry& spawnEntry);
    void RemoveSpawn(uint16 bubbleID, uint32 itemID);

    uint8 GetSpawnGroup(uint8 sClass);

    uint16 GetRandTypeID(uint8 sClass);

    typedef std::vector<NPC*> RatSpawningVec;
    typedef std::vector<SystemBubble*> RatBubbleVec;
    typedef std::vector<SpawnGroup> RatSpawnGroupVec;
    typedef std::vector<RatSpawnClass> RatSpawnClassVec;
    typedef std::vector<RatFactionGroups> RatFactionGroupsVec;
    typedef std::map<uint8, uint16> RatFactionGroupsMap;    //map to enable 'find'  shipClass is key
    typedef std::multimap<uint16, SpawnEntry> SpawnEntryDef;    //bubbleID is key
    //typedef std::vector<uint32, SystemSpawnGroup> SystemSpawnGroupVec;  //systemID is key  *unused at this time*

private:
    SystemManager* m_system;    //we do not own this
    EVEServiceManager& m_services;    //we do not own this
    DungeonMgr* m_dungMgr;    //we do not own this

    Timer m_ratTimer;
    Timer m_ratGroupTimer;
    Timer m_missionTimer;
    Timer m_incursionTimer;
    Timer m_deadspaceTimer;

    bool m_initalized;      //allow spawning?

    uint16 m_groupTimerSetTime;     //ms - this is for hard-coding the respawn timer time.

    uint32 m_spawnID;       //in case i need to track a specific spawn group.

    RatBubbleVec m_bubbles;
    SpawnEntryDef m_spawns;
    std::map<uint32, uint32> m_incursionAlive;   // bubbleID -> NPC count for incursion wave tracking
    // incursion wave chain (user rule: gates + waves). SpawnMgr tracks the
    // dungeon/wave per site bubble; when wave N's NPCs all die, the NEXT wave
    // spawns in a new pocket 50M km along +x and a real 17831 Acceleration Gate
    // (the mechanism sleepers use) spawns ~30km past the cleared pocket and
    // teleports into the new pocket on activation. After the LAST wave, the
    // regular site-completion/reward path runs as before.
    struct IncursionWave {
        uint32 dungeonID = 0;
        uint8  waveNum   = 1;
        GPoint pocket;
    };
    std::map<uint32, IncursionWave> m_incursionWave;   // bubbleID -> wave state
    RatSpawningVec m_ratSpawns;
    RatSpawnGroupVec m_toSpawn;
    RatSpawnClassVec m_ratSpawnClass;               // not used
    //SystemSpawnGroupVec m_spawnGroups;
    RatFactionGroupsMap m_factionGroups;
};


#endif  // _EVE_NPC_SPAWNMGR_H__
