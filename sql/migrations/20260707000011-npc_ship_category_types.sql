-- Create Ship-category clone typeIDs for Entity NPCs (fixes crosshair rendering)
-- Client's built-in SDE has Entity NPCs as cat 11, which disables crosshairs
-- New Ship-category types force client to render Ship-style brackets with crosshairs
-- +migrate Up

-- Sansha NPC clones (Ship category, groups 25/26/27/419)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(34000, 25, 'Sansha Frigate', '', 1238, 50, 2112000, 21120, 100, 1, 16, 0, 0, 31),
(34001, 26, 'Sansha Cruiser', '', 1236, 150, 10010000, 100100, 200, 1, 16, 0, 0, 31),
(34002, 27, 'Sansha Battleship', '', 2295, 350, 10010000, 100100, 235, 1, 16, 0, 0, 31),
(34003, 419, 'Sansha Battlecruiser', '', 1236, 150, 10010000, 100100, 235, 1, 16, 0, 0, 31);

-- Copy dogma attributes from SDE Entity types to Ship clones
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat)
SELECT 34000, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat)
SELECT 34001, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat)
SELECT 34002, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat)
SELECT 34003, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23383;

-- Copy dogma effects from SDE Entity types to Ship clones
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault)
SELECT 34000, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault)
SELECT 34001, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault)
SELECT 34002, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault)
SELECT 34003, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23383;

-- +migrate Down
DELETE FROM dgmTypeEffects WHERE typeID IN (34000, 34001, 34002, 34003);
DELETE FROM dgmTypeAttributes WHERE typeID IN (34000, 34001, 34002, 34003);
DELETE FROM invTypes WHERE typeID IN (34000, 34001, 34002, 34003);
