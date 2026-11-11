-- Fill the pirate/NPC faction for COSMOS hunt objectives that the wiki parser
-- left blank, using the faction that owns the constellation's region.
-- +migrate Up
UPDATE missionObjectives mo
JOIN cosmosAgentMissions cam ON cam.missionID = mo.missionID
JOIN agtAgents a ON a.agentID = cam.agentID
JOIN mapSolarSystems s ON s.solarSystemID = a.locationID
JOIN mapRegions r ON r.regionID = s.regionID
SET mo.npcFactionID = r.factionID
WHERE mo.missionID BETWEEN 93000 AND 93580
  AND mo.objectiveType = 3
  AND (mo.npcFactionID IS NULL OR mo.npcFactionID = 0)
  AND r.factionID <> 0;

-- +migrate Down
-- (faction hints stay)
