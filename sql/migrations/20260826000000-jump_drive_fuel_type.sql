-- Capital jump drive fuel type (AttrJumpDriveConsumptionType 866) by race.
-- The client's jump-drive window (capitalnavigation.py) reads this attribute to
-- show the required fuel; without it the fuel column was empty and the server's
-- fuel check (fuelType = AttrJumpDriveConsumptionType = 0) never validated.
-- Each empire's capitals burn their own isotope:
--   race 1 Caldari  -> Helium Isotopes (16274)
--   race 2 Minmatar -> Hydrogen Isotopes (17889)
--   race 4 Amarr    -> Nitrogen Isotopes (17888)
--   race 8 Gallente -> Oxygen Isotopes (17887)
--   other (Jove)    -> Helium Isotopes (default)
-- +migrate Up
INSERT INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat)
SELECT t.typeID, 866,
       CASE t.raceID
           WHEN 1 THEN 16274
           WHEN 2 THEN 17889
           WHEN 4 THEN 17888
           WHEN 8 THEN 17887
           ELSE 16274
       END,
       NULL
FROM invTypes t
JOIN invGroups g ON g.groupID = t.groupID
WHERE g.groupName IN ('Supercarrier','Carrier','Titan','Dreadnought','JumpFreighter','Freighter')
  AND NOT EXISTS (SELECT 1 FROM dgmTypeAttributes ta WHERE ta.typeID = t.typeID AND ta.attributeID = 866);

-- +migrate Down
DELETE ta FROM dgmTypeAttributes ta
JOIN invTypes t ON t.typeID = ta.typeID
JOIN invGroups g ON g.groupID = t.groupID
WHERE ta.attributeID = 866
  AND g.groupName IN ('Supercarrier','Carrier','Titan','Dreadnought','JumpFreighter','Freighter');
