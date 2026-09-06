#ifndef _EVE_SERVER_MISSIONS_ENCOUNTER_SERVER_H__
#define _EVE_SERVER_MISSIONS_ENCOUNTER_SERVER_H__

#include "../eve-server.h"
#include "../services/ServiceManager.h"

struct MissionOffer;

struct MissionEncounter {
    uint32 encounterID;
    uint32 missionID;
    uint32 agentID;
    uint32 charID;
    uint32 systemID;
    uint32 offerID;              // agtOffers.offerID this encounter belongs to (completion target)
    uint8 agentTypeID;
    std::string encounterName;
    std::string anomSigID;  // anomaly sigID for FW mission visibility
    GPoint targetPos;        // where the target cluster was spawned (client warps here)
    std::vector<uint32> spawnedEntities;
    bool active;
};

class EncounterSpawnServer : public Service<EncounterSpawnServer> {
public:
    EncounterSpawnServer();
    ~EncounterSpawnServer();

    PyResult GetMyEncounters(PyCallArgs& call);
    PyResult RequestActivateEncounters(PyCallArgs& call, PyList* encounterList);
    PyResult RequestDeactivateEncounters(PyCallArgs& call, PyList* encounterList);

    void AddEncounter(uint32 charID, uint32 missionID, uint32 agentID, uint8 agentTypeID, const std::string& name);
    void RemoveEncounter(uint32 encounterID);

    // Server-side mission support (no client needed): an accepted Encounter
    // offer registers its target cluster here so the agent can mark the mission
    // complete once every spawned target has been destroyed.
    static EncounterSpawnServer* Get();          // singleton access (set in ctor)
    // Spawn this offer's hostile NPC target cluster (if any) and record it.
    // Returns the number of targets spawned (0 = nothing to do / no system).
    uint32 SpawnEncounterForOffer(MissionOffer& offer, uint32 destinationSystemID, const GPoint& atPos);
    // Called from NPC::Killed when a mission-flagged NPC dies. Removes the dead
    // entity from its encounter; when none remain the offer is marked cleared
    // (agtOffers.missionNPCsKilled = missionNPCs).
    void OnMissionTargetKilled(uint32 entityID);
    // True when all targets of the encounter for this offer are dead (usable by
    // Client::IsMissionComplete for Encounter missions).
    bool IsOfferComplete(uint32 offerID) const;
    // Register a mining mission's ore belt position (spawned on Accept). Kept so
    // GetTargetPosition also serves Mining missions (client warp link + journal
    // objective), without any NPC kill-tracking.
    void RegisterMiningSite(uint32 offerID, const GPoint& pos);
    // Position the target cluster was spawned at for this offer (for building the
    // client's warp-to-site objective). Returns false if the offer is untracked.
    bool GetTargetPosition(uint32 offerID, GPoint& outPos) const;

private:
    void DespawnEncounters(uint32 encounterID);
    void MarkOfferCleared(MissionEncounter& enc);

    std::map<uint32, MissionEncounter> m_encounters;
    std::map<uint32, GPoint> m_miningSites;   // offerID -> ore belt position (mining missions)
    uint32 m_nextEncounterID;
    static EncounterSpawnServer* s_instance;
};

#endif
