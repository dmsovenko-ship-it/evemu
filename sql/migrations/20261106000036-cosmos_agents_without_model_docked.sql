-- 11 COSMOS agents have a mission chain but no in-space model (group 517 attr 840);
-- dock them so their chains are still reachable at a station.
-- +migrate Up
UPDATE agtAgents a
  JOIN (SELECT solarSystemID, MIN(stationID) AS sid FROM staStations GROUP BY solarSystemID) s
    ON s.solarSystemID = a.locationID
  SET a.locationID = s.sid
  WHERE a.agentID IN (3018423,3018460,3018714,3018715,3018718,3018720,3018725,3018771,3018772,3018774,3018775) AND a.locationID BETWEEN 30000000 AND 30999999;
UPDATE agtAgents a
  JOIN mapSolarSystems ms ON ms.solarSystemID = a.locationID
  JOIN (SELECT ms2.constellationID, MIN(st.stationID) AS sid FROM staStations st
          JOIN mapSolarSystems ms2 ON ms2.solarSystemID = st.solarSystemID
        GROUP BY ms2.constellationID) c ON c.constellationID = ms.constellationID
  SET a.locationID = c.sid
  WHERE a.agentID IN (3018423,3018460,3018714,3018715,3018718,3018720,3018725,3018771,3018772,3018774,3018775) AND a.locationID BETWEEN 30000000 AND 30999999;
UPDATE agtAgents a
  JOIN mapSolarSystems ms ON ms.solarSystemID = a.locationID
  JOIN (SELECT ms2.regionID, MIN(st.stationID) AS sid FROM staStations st
          JOIN mapSolarSystems ms2 ON ms2.solarSystemID = st.solarSystemID
        GROUP BY ms2.regionID) r ON r.regionID = ms.regionID
  SET a.locationID = r.sid
  WHERE a.agentID IN (3018423,3018460,3018714,3018715,3018718,3018720,3018725,3018771,3018772,3018774,3018775) AND a.locationID BETWEEN 30000000 AND 30999999;
UPDATE chrNPCCharacters c
  JOIN agtAgents a ON a.agentID = c.characterID
  LEFT JOIN staStations s ON s.stationID = a.locationID
  LEFT JOIN mapSolarSystems ms ON ms.solarSystemID = IFNULL(s.solarSystemID, a.locationID)
  SET c.stationID = IF(a.locationID BETWEEN 60000000 AND 60999999, a.locationID, 0),
      c.solarSystemID = IFNULL(s.solarSystemID, c.solarSystemID),
      c.constellationID = IFNULL(ms.constellationID, c.constellationID),
      c.regionID = IFNULL(ms.regionID, c.regionID)
  WHERE a.agentID IN (3018423,3018460,3018714,3018715,3018718,3018720,3018725,3018771,3018772,3018774,3018775);
-- +migrate Down
-- (agents stay docked)
