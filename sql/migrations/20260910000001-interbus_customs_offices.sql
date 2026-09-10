-- +migrate Up
-- NPC InterBus customs offices in high-sec (like the official server, where
-- InterBus operates planetary customs offices in empire space). One office per
-- high-sec planet, bound to that planet (customInfo = planet itemID) so the
-- CustomsSE can anchor/serve PI export-import. Players may still anchor their
-- own (typeID 2233) and take over/compete.
INSERT INTO entity
    (itemName, typeID, ownerID, locationID, flag, contraband, singleton, quantity,
     x, y, z, customInfo, isActive)
SELECT 'Customs Office', 4318, 1000148, p.solarSystemID, 0, 0, 1, 1,
       p.x + IFNULL(p.radius, 0) + 50000, p.y, p.z, CAST(p.itemID AS CHAR), 1
FROM mapDenormalize p
JOIN mapSolarSystems s ON s.solarSystemID = p.solarSystemID
WHERE p.groupID = 7                 -- planets
  AND s.security >= 0.5             -- high-sec only
  AND NOT EXISTS (
      SELECT 1 FROM entity e
      WHERE e.typeID = 4318 AND e.customInfo = CAST(p.itemID AS CHAR));

-- +migrate Down
DELETE FROM entity WHERE typeID = 4318 AND ownerID = 1000148;
