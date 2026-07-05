#include "missions/EncounterServer.h"
#include "missions/MissionDataMgr.h"
#include "npc/NPC.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "system/DestinyManager.h"

EncounterSpawnServer::EncounterSpawnServer()
: Service("encounterSpawnServer"),
  m_nextEncounterID(1)
{
    this->Add("GetMyEncounters", &EncounterSpawnServer::GetMyEncounters);
    this->Add("RequestActivateEncounters", &EncounterSpawnServer::RequestActivateEncounters);
    this->Add("RequestDeactivateEncounters", &EncounterSpawnServer::RequestDeactivateEncounters);
}

EncounterSpawnServer::~EncounterSpawnServer()
{
}

void EncounterSpawnServer::AddEncounter(uint32 charID, uint32 missionID, uint32 agentID, const std::string& name)
{
    MissionEncounter enc;
    enc.encounterID = m_nextEncounterID++;
    enc.missionID = missionID;
    enc.agentID = agentID;
    enc.charID = charID;
    enc.encounterName = name;
    enc.active = false;
    m_encounters.emplace(enc.encounterID, enc);
}

void EncounterSpawnServer::RemoveEncounter(uint32 encounterID)
{
    auto it = m_encounters.find(encounterID);
    if (it != m_encounters.end()) {
        m_encounters.erase(it);
    }
}

PyResult EncounterSpawnServer::GetMyEncounters(PyCallArgs& call)
{
    _log(AGENT__INFO, "EncounterSpawnServer::GetMyEncounters() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    uint32 charID = call.client->GetCharID();
    PyList* result = new PyList();

    for (auto& pair : m_encounters) {
        MissionEncounter& enc = pair.second;
        if (enc.charID == charID) {
            PyDict* entry = new PyDict();
            entry->SetItem(new PyString("encounterID"), new PyInt(enc.encounterID));
            entry->SetItem(new PyString("encounterName"), new PyString(enc.encounterName));
            entry->SetItem(new PyString("agentID"), new PyInt(enc.agentID));
            entry->SetItem(new PyString("active"), enc.active ? PyStatic.NewTrue() : PyStatic.NewFalse());
            result->AddItem(entry);
        }
    }

    return result;
}

PyResult EncounterSpawnServer::RequestActivateEncounters(PyCallArgs& call, PyList* encounterList)
{
    _log(AGENT__INFO, "EncounterSpawnServer::RequestActivateEncounters() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    PyList* result = new PyList();
    PyDict* logResult = new PyDict();
    logResult->SetItem(new PyString("log"), PyStatic.NewNone());

    _log(AGENT__MESSAGE, "EncounterSpawnServer: encounter activation request received (%lu encounters)", encounterList->size());
    for (size_t i = 0; i < encounterList->size(); ++i) {
        PyRep* item = encounterList->GetItem(i);
        if (item->IsInt()) {
            result->AddItem(new PyInt(item->AsInt()->value()));
        }
    }

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, result);
    rsp->SetItem(1, logResult);
    return rsp;
}

PyResult EncounterSpawnServer::RequestDeactivateEncounters(PyCallArgs& call, PyList* encounterList)
{
    _log(AGENT__INFO, "EncounterSpawnServer::RequestDeactivateEncounters() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    PyList* result = new PyList();
    PyDict* logResult = new PyDict();
    logResult->SetItem(new PyString("log"), PyStatic.NewNone());

    _log(AGENT__MESSAGE, "EncounterSpawnServer: encounter deactivation request received (%lu encounters)", encounterList->size());
    for (size_t i = 0; i < encounterList->size(); ++i) {
        PyRep* item = encounterList->GetItem(i);
        if (item->IsInt()) {
            result->AddItem(new PyInt(item->AsInt()->value()));
        }
    }

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, result);
    rsp->SetItem(1, logResult);
    return rsp;
}
