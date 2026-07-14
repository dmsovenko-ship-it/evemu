
 /**
  * @name AgentDB.cpp
  *   Agent DB operations
  *    original agent code by zhur, this was completely rewritten based on new data.
  *
  * @Author:        Allan
  * @date:      24 June 2018
  *
  */

/** @todo  fix this....not all agents have an entry in chrNPCCharacters table.  */

#include "eve-server.h"

#include "agents/AgentDB.h"
#include "StaticDataMgr.h"


void AgentDB::LoadAgentData(uint32 agentID, AgentData& data)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "   agt.agentTypeID,"
        "   agt.divisionID,"
        "   agt.level,"
        "   agt.quality,"
        "   agt.corporationID,"
        "   agt.locationID, "   //5
        "   agt.isLocator,"
        "   COALESCE(chr.solarSystemID, itm.solarSystemID, agt.locationID) AS solarSystemID,"
        "   COALESCE(chr.stationID, CASE WHEN agt.locationID BETWEEN 60000000 AND 60999999 THEN agt.locationID ELSE 0 END) AS stationID,"
        "   chr.gender,"
        "   bl.bloodlineID,"    //10
        "   itm.typeID, "
        "   crp.friendID, "
        "   crp.enemyID, "
        "   crp.factionID, "
        "   chr.characterName " //15
        " FROM agtAgents AS agt"
        " LEFT JOIN chrNPCCharacters AS chr ON chr.characterID = agt.agentID"
        " LEFT JOIN crpNPCCorporations AS crp ON crp.corporationID = agt.corporationID"
        " LEFT JOIN bloodlineTypes AS bl ON bl.typeID = chr.typeID"
        " LEFT JOIN mapDenormalize AS itm ON itm.itemID = agt.locationID"
        " WHERE agt.agentID = %u", agentID))
    {
        codelog(DATABASE__ERROR, "Error in GetAgents query: %s", res.error.c_str());
        return;
    }

    /** @todo  there may be some errors here with agents in space or NOT in stations....will have to test and fix as they come up.  */
    DBResultRow row;
    if (res.GetRow(row)) {
        if (row.IsNull(10) or row.GetUInt(10) == 0) {
            _log(DATABASE__MESSAGE, "No charTypeID for Agent %u", agentID);
            return;
        }
        data.typeID         = row.GetUInt(0);
        data.divisionID     = row.GetUInt(1);
        data.level          = row.GetUInt(2);
        data.quality        = row.IsNull(3) ? 0 : row.GetInt(3);
        data.corporationID  = row.GetUInt(4);
        data.locationID     = row.GetUInt(5);
        data.locator        = (row.IsNull(6) ? false : row.GetBool(6));
        data.solarSystemID  = row.GetUInt(7);
        data.stationID      = row.GetUInt(8);
        data.gender         = (row.IsNull(9) ? false : row.GetBool(9));
        data.bloodlineID    = row.GetUInt(10);
        data.locationTypeID = row.GetUInt(11);
        data.friendCorp     = (row.IsNull(12) ? 0 : row.GetInt(12));
        data.enemyCorp      = (row.IsNull(13) ? 0 : row.GetInt(13));
        data.factionID      = (row.IsNull(14) ? 0 : row.GetInt(14));
        data.raceID         = sDataMgr.GetFactionRace(data.factionID);
        data.name           = (row.IsNull(15) ? "" : row.GetText(15));
        data.research       = (data.typeID == Agents::Type::Research);
        data.cosmos         = (data.typeID == Agents::Type::Cosmos);
        data.facWar         = (data.typeID == Agents::Type::FacWar);
    }
}

void AgentDB::LoadAgentSkills(uint32 agentID, std::map< uint16, uint8 >& data)
{
    // SELECT typeID, level FROM agtSkillLevel WHERE agentID = %u
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT typeID, level FROM agtSkillLevel WHERE agentID = %u", agentID))
        codelog(DATABASE__ERROR, "Error in LoadAgentSkills query: %s", res.error.c_str());

    DBResultRow row;
    while (res.GetRow(row))
        data[row.GetInt(0)] = row.GetInt(1);
}

PyRep* AgentDB::GetCareerAgentMapping()
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT careerType, agentID"
        " FROM tutorial_career_agents"))
    {
        codelog(DATABASE__ERROR, "Error in GetCareerAgentMapping query: %s", res.error.c_str());
        return nullptr;
    }
    PyDict* result = new PyDict();
    DBResultRow row;
    std::map<std::string, PyList*> groups;
    while (res.GetRow(row)) {
        std::string careerType = row.GetText(0);
        uint32 agentID = row.GetUInt(1);
        auto it = groups.find(careerType);
        if (it == groups.end()) {
            PyList* lst = new PyList();
            lst->AddItem(new PyInt(agentID));
            groups.emplace(careerType, lst);
        } else {
            it->second->AddItem(new PyInt(agentID));
        }
    }
    for (auto& pair : groups)
        result->SetItemString(pair.first.c_str(), pair.second);
    return result;
}
