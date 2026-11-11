-- Epic arc starting agents live in space in the SDE but have no in-space model;
-- dock them at a station of their system so their arcs can actually start.
-- +migrate Up
UPDATE agtAgents a
  JOIN (SELECT solarSystemID, MIN(stationID) AS sid FROM staStations GROUP BY solarSystemID) s
    ON s.solarSystemID = a.locationID
  SET a.locationID = s.sid
  WHERE a.agentID IN (3019465,3019463,3019415,3019396,3019412,3019411)
    AND a.locationID BETWEEN 30000000 AND 30999999;
UPDATE chrNPCCharacters c
  JOIN agtAgents a ON a.agentID = c.characterID
  LEFT JOIN staStations s ON s.stationID = a.locationID
  LEFT JOIN mapSolarSystems ms ON ms.solarSystemID = IFNULL(s.solarSystemID, a.locationID)
  SET c.stationID = IF(a.locationID BETWEEN 60000000 AND 60999999, a.locationID, 0),
      c.solarSystemID = IFNULL(s.solarSystemID, c.solarSystemID),
      c.constellationID = IFNULL(ms.constellationID, c.constellationID),
      c.regionID = IFNULL(ms.regionID, c.regionID)
  WHERE a.agentID IN (3019465,3019463,3019415,3019396,3019412,3019411);
-- Vision of Greatness has no SDE agent: name its synthetic start agent
UPDATE chrNPCCharacters SET characterName='Cassandra Ahfrim'
  WHERE characterID=3019514 AND characterName LIKE 'Agent %';
-- +migrate Down
-- (agents stay docked)
