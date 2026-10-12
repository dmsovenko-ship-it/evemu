-- +migrate Up
-- Crucible: InterBus held planetary customs offices in ALL space - empire,
-- low-sec, null-sec and wormhole systems (players destroyed 5,128 of them in
-- the first week alone, 778 low / 2,298 null / 2,052 w-space, with no
-- security/standing penalty). InterBus never rebuilt a destroyed office; the
-- planet was simply left without one until a player anchored their own.
--
-- The original seed (20260910000001) covered high-sec only. Extend it to every
-- planet. One office per planet (customInfo = planet itemID); skipped wherever
-- an office already exists (InterBus 4318 or corp 2233).
INSERT INTO entity
    (itemName, typeID, ownerID, locationID, flag, contraband, singleton, quantity,
     x, y, z, customInfo, isActive)
SELECT 'Customs Office', 4318, 1000148, p.solarSystemID, 0, 0, 1, 1,
       p.x + IFNULL(p.radius, 0) + 50000, p.y, p.z, CAST(p.itemID AS CHAR), 1
FROM mapDenormalize p
WHERE p.groupID = 7                 -- planets
  AND NOT EXISTS (
      SELECT 1 FROM entity e
      WHERE e.typeID IN (4318, 2233) AND e.customInfo = CAST(p.itemID AS CHAR));

-- +migrate Down
-- Cannot distinguish the rows added here from the earlier high-sec seed;
-- destroying offices is an in-game action, not a schema operation - leave them.
