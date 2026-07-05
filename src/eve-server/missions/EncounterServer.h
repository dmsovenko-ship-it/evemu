#ifndef _EVE_SERVER_MISSIONS_ENCOUNTER_SERVER_H__
#define _EVE_SERVER_MISSIONS_ENCOUNTER_SERVER_H__

#include "../eve-server.h"

struct MissionEncounter {
    uint32 encounterID;
    uint32 missionID;
    uint32 agentID;
    uint32 charID;
    std::string encounterName;
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

    void AddEncounter(uint32 charID, uint32 missionID, uint32 agentID, const std::string& name);
    void RemoveEncounter(uint32 encounterID);

private:
    uint32 SpawnNPCs(uint32 charID, uint32 systemID, const std::string& faction);
    void DespawnNPCs(uint32 encounterID);

    std::map<uint32, MissionEncounter> m_encounters;
    uint32 m_nextEncounterID;
};

#endif
