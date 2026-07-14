-- Fix stale courier mission offers with zero/system destination/origin IDs
-- Created when agents had no chrNPCCharacters entry (stationID=0 in MakeOffer)
-- Also fixes cases where agt.locationID is a solarSystemID, not a stationID
-- +migrate Up

UPDATE agtOffers o
  INNER JOIN agtAgents a ON o.agentID = a.agentID
  LEFT JOIN chrNPCCharacters c ON c.characterID = a.agentID
  LEFT JOIN mapDenormalize m ON m.itemID = a.locationID
  SET
    o.originID           = COALESCE(NULLIF(c.stationID, 0),
                              CASE WHEN a.locationID BETWEEN 60000000 AND 60999999 THEN a.locationID
                                ELSE (SELECT stationID FROM staStations WHERE solarSystemID = m.solarSystemID LIMIT 1)
                              END, 0),
    o.originOwnerID      = COALESCE(a.corporationID, 0),
    o.originSystemID     = COALESCE(NULLIF(c.solarSystemID, 0), m.solarSystemID, a.locationID, 0),
    o.destinationID      = COALESCE(NULLIF(c.stationID, 0),
                              CASE WHEN a.locationID BETWEEN 60000000 AND 60999999 THEN a.locationID
                                ELSE (SELECT stationID FROM staStations WHERE solarSystemID = m.solarSystemID LIMIT 1)
                              END, 0),
    o.destinationOwnerID = COALESCE(a.corporationID, 0),
    o.destinationSystemID = COALESCE(NULLIF(c.solarSystemID, 0), m.solarSystemID, a.locationID, 0),
    o.courierTypeID      = CASE WHEN o.courierTypeID > 0 THEN o.courierTypeID ELSE 23 END,
    o.courierAmount      = CASE WHEN o.courierAmount > 0 THEN o.courierAmount ELSE 1 END
  WHERE o.typeID = 3
    AND (o.destinationID < 60000000 OR o.destinationID > 60999999
      OR o.originID < 60000000 OR o.originID > 60999999
      OR o.courierTypeID = 0);

-- +migrate Down
-- No down migration — this only fixes data that was already incorrect
