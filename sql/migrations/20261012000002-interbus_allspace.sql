-- +migrate Up
-- Crucible: InterBus held planetary customs offices in ALL space - empire,
-- low-sec, null-sec and wormhole systems (players destroyed 5,128 of them in
-- the first week alone, 778 low / 2,298 null / 2,052 w-space, with no
-- security/standing penalty). InterBus never rebuilt a destroyed office; the
-- planet was simply left without one until a player anchored their own.
--
-- The original seed (20260910000001) covered high-sec only. Extend it to every
-- planet. One office per planet (customInfo = planet itemID). The set of
-- planets that already hold an office is materialised first: a correlated
-- NOT EXISTS against the INSERT target table is unreliable here.
CREATE TEMPORARY TABLE _planets_with_office AS
    SELECT DISTINCT customInfo AS c FROM entity WHERE typeID IN (4318, 2233);
CREATE INDEX _pwo ON _planets_with_office (c);

INSERT INTO entity
    (itemName, typeID, ownerID, locationID, flag, contraband, singleton, quantity,
     x, y, z, customInfo, isActive)
SELECT 'Customs Office', 4318, 1000148, p.solarSystemID, 0, 0, 1, 1,
       p.x + IFNULL(p.radius, 0) + 50000, p.y, p.z, CAST(p.itemID AS CHAR), 1
FROM mapDenormalize p
LEFT JOIN _planets_with_office h ON h.c = CAST(p.itemID AS CHAR)
WHERE p.groupID = 7                 -- planets
  AND h.c IS NULL;

DROP TEMPORARY TABLE _planets_with_office;

-- +migrate Down
-- Cannot distinguish the rows added here from the earlier high-sec seed;
-- destroying offices is an in-game action, not a schema operation - leave them.
