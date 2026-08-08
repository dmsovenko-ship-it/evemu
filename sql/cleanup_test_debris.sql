-- Cleanup test debris from entity table:
--   1. Orphaned drones floating in space (flag=0, locationID = solar system, isActive=0)
--      from test sessions / crashes / relogs — includes owner=1 (EVE system) leftovers.
--      Player drones in a ship's drone bay (flag=87) are untouched.
--   2. Orphaned drone-bay drones whose locationID points at a solar system (not a ship) —
--      stale entries from old ships/relogs.
--   3. Dungeon decorations (containers/clouds/wrecks/asteroids) previously persisted
--      by SpawnDecorations — now spawned transient, these saved copies are stale.
DELETE FROM entity_attributes
WHERE itemID IN (
    SELECT itemID FROM entity WHERE flag = 0 AND isActive = 0
      AND typeID IN (2175, 2456, 2486, 2203, 2463, 2205, 28211, 2454, 2446, 2478)
);
DELETE FROM entity_attributes
WHERE itemID IN (
    SELECT itemID FROM entity
      WHERE typeID IN (3298, 3293, 3296, 3465, 3467, 17366, 24445, 26468, 10753, 10754, 10758,
                       1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 24545, 19373)
        AND ownerID = 1
);
DELETE FROM entity_attributes
WHERE itemID IN (
    SELECT itemID FROM entity WHERE flag = 87 AND isActive = 0
      AND typeID IN (2175, 2456, 2486, 2203, 2463, 2205, 28211, 2454, 2446, 2478)
);

DELETE FROM entity
WHERE flag = 0 AND isActive = 0
  AND typeID IN (2175, 2456, 2486, 2203, 2463, 2205, 28211, 2454, 2446, 2478);

DELETE FROM entity
WHERE typeID IN (3298, 3293, 3296, 3465, 3467, 17366, 24445, 26468, 10753, 10754, 10758,
                 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 24545, 19373)
  AND ownerID = 1;

DELETE FROM entity
WHERE flag = 87 AND isActive = 0
  AND typeID IN (2175, 2456, 2486, 2203, 2463, 2205, 28211, 2454, 2446, 2478);

