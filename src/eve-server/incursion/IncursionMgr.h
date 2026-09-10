#ifndef __INCURSION_MGR_H_INCL__
#define __INCURSION_MGR_H_INCL__

#include "utils/Singleton.h"
#include "utils/timer.h"
#include <map>
#include <set>
#include <unordered_map>

class SystemManager;

class IncursionMgr : public Singleton<IncursionMgr>
{
public:
    IncursionMgr();
    void Process();

    void StartIncursion(uint32 factionID, uint32 constellationID);
    void EndIncursion(uint32 incursionID);
    void OnSiteCompleted(uint32 incursionID, uint32 solarSystemID, uint8 sceneType);
    void SpawnMothership(uint32 incursionID, uint32 solarSystemID);
    static bool IsIncursionSystem(uint32 solarSystemID);
    static uint8 GetSceneType(uint32 solarSystemID, uint32 incursionID = 0);

    // Incursion contest: track damage per player per bubble
    void RecordDamage(uint32 bubbleID, uint32 charID, double damage);
    std::map<uint32, double>& GetBubbleDamage(uint32 bubbleID);
    void ClearDamageData(uint32 bubbleID);

    // Push incursion tale data to clients (0 = all incursions). Called on
    // session change so the HUD appears when entering an incursed system.
    void NotifyClients(uint32 incursionID);

    // Testing/GM: force-spawn an incursion site in the given system (if an
    // incursion covers it), ignoring the random chance and player gate.
    bool ForceSpawnSiteHere(uint32 solarSystemID);
    // Testing/GM: start an incursion in the system's constellation if none, then
    // force a site into the system.
    bool ForceStartHere(uint32 solarSystemID);

private:
    void ProgressStateMachine(uint32 incursionID);
    void UpdateInfluence(uint32 incursionID);
    void SpawnSites(uint32 incursionID);
    void DespawnSites(uint32 incursionID);

    Timer m_spawnTimer;
    std::set<uint32> m_activeSystems;  // solarSystemIDs that currently have active incursion sites
    // Per-bubble damage tracking for contest rewards: bubbleID -> (charID -> totalDamage)
    std::map<uint32, std::map<uint32, double>> m_bubbleDamage;
};

#define sIncursionMgr \
    (IncursionMgr::get())

#endif
