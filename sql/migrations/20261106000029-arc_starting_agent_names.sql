-- Arc starting agents (synthetic wrapper agents 3019502..3019514) and the
-- remaining nameless FW agents: give them a real name.
-- +migrate Up
INSERT IGNORE INTO chrNPCCharacters
  (characterID, characterName, description, typeID, gender, stationID, solarSystemID, constellationID, regionID)
SELECT a.agentID, CONCAT('Agent ', a.agentID), '', 1376, 0, 0, 0, 0, 0
  FROM agtAgents a
  LEFT JOIN chrNPCCharacters c ON c.characterID = a.agentID
  WHERE c.characterID IS NULL;
UPDATE chrNPCCharacters c
  JOIN agtAgents a ON a.agentID = c.characterID
  LEFT JOIN staStations s ON s.stationID = a.locationID
  LEFT JOIN mapSolarSystems ms ON ms.solarSystemID = IFNULL(s.solarSystemID, a.locationID)
  SET c.stationID = IF(a.locationID BETWEEN 60000000 AND 60999999, a.locationID, 0),
      c.solarSystemID = IFNULL(s.solarSystemID, IF(a.locationID BETWEEN 30000000 AND 30999999, a.locationID, 0)),
      c.constellationID = IFNULL(ms.constellationID, c.constellationID),
      c.regionID = IFNULL(ms.regionID, c.regionID)
  WHERE c.characterName <> '';
-- +migrate Down
-- (names stay)
