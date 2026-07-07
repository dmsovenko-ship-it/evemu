-- Create Ship-category clone typeIDs for Entity NPCs (fixes crosshair rendering)
-- Client's built-in SDE caches Entity NPCs as cat 11 → no crosshairs
-- New Ship-category typeIDs are unknown to client → loaded from server cache → Ship cat → crosshairs
-- +migrate Up

-- All pirate factions, each with 4 ship classes: Frigate(25), Cruiser(26), Battleship(27), Battlecruiser(419)
-- Mass/volume/capacity copied from SDE Entity types for consistency

-- Sansha (typeIDs 37000-37003, src: 10025/10030/11913/23383)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37000, 25,  'Sansha Frigate', '', 1238, 50,  2112000, 21120, 100, 1, 16, 0, 0, 31),
(37001, 26,  'Sansha Cruiser', '', 1236, 150, 10010000, 100100, 200, 1, 16, 0, 0, 31),
(37002, 27,  'Sansha Battleship', '', 2295, 350, 10010000, 100100, 235, 1, 16, 0, 0, 31),
(37003, 419, 'Sansha Battlecruiser', '', 1236, 150, 10010000, 100100, 235, 1, 16, 0, 0, 31);

-- Angel (37100-37103, src: 2372/10017/11898/22822)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37100, 25,  'Angel Frigate', '', 344, 50,  2250000, 22500, 75, 1, 4, 0, 0, 31),
(37101, 26,  'Angel Cruiser', '', 336, 150, 9900000, 99000, 1900, 1, 4, 0, 0, 31),
(37102, 27,  'Angel Battleship', '', 335, 350, 19000000, 19000000, 120, 1, 4, 0, 0, 31),
(37103, 419, 'Angel Battlecruiser', '', 26378, 150, 10900000, 109000, 120, 1, 4, 0, 0, 31);

-- Blood Raiders (37104-37107, src: 10275/10281/11905/23244)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37104, 25,  'Blood Frigate', '', 1762, 50,  2810000, 28100, 120, 1, 8, 0, 0, 31),
(37105, 26,  'Blood Cruiser', '', 1760, 150, 11500000, 115000, 465, 1, 8, 0, 0, 31),
(37106, 27,  'Blood Battleship', '', 2122, 350, 20500000, 1100000, 235, 1, 8, 0, 0, 31),
(37107, 419, 'Blood Battlecruiser', '', 2795, 150, 11800000, 118000, 235, 1, 8, 0, 0, 31);

-- Guristas (37108-37111, src: 2382/2387/11932/23321)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37108, 25,  'Guristas Frigate', '', 1968, 50,  1500100, 15001, 45, 1, 1, 0, 0, 31),
(37109, 26,  'Guristas Cruiser', '', 1823, 150, 10700000, 107000, 850, 1, 1, 0, 0, 31),
(37110, 27,  'Guristas Battleship', '', 2159, 350, 21000000, 1040000, 235, 1, 1, 0, 0, 31),
(37111, 419, 'Guristas Battlecruiser', '', 26204, 150, 10100000, 101000, 235, 1, 1, 0, 0, 31);

-- Serpentis (37112-37115, src: 2370/2381/11921/23438)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37112, 25,  'Serpentis Frigate', '', 1816, 50,  2450000, 24500, 60, 1, 2, 0, 0, 31),
(37113, 26,  'Serpentis Cruiser', '', 1812, 150, 11600000, 116000, 900, 1, 2, 0, 0, 31),
(37114, 27,  'Serpentis Battleship', '', 2156, 350, 19000000, 1010000, 480, 1, 2, 0, 0, 31),
(37115, 419, 'Serpentis Battlecruiser', '', 2917, 150, 11200000, 112000, 480, 1, 2, 0, 0, 31);

-- Rogue Drones (37116-37119, src: 25636/25632/25648/25640)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(37116, 25,  'Rogue Drone Frigate', '', 1226, 45, 100000, 60, 1200, 1, 0, 0, 0, 0),
(37117, 26,  'Rogue Drone Cruiser', '', 1222, 150, 100000, 60, 1200, 1, 0, 0, 0, 0),
(37118, 27,  'Rogue Drone Battleship', '', 1718, 350, 21000000, 1010000, 950, 1, 0, 0, 0, 0),
(37119, 419, 'Rogue Drone Battlecruiser', '', 1223, 150, 100000, 60, 1200, 1, 0, 0, 0, 0);

-- Copy dogma attributes and effects for all factions
-- Sansha
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37000, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37001, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37002, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37003, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23383;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37000, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37001, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37002, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37003, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23383;

-- Angel
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37100, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2372;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37101, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10017;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37102, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11898;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37103, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 22822;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37100, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2372;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37101, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10017;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37102, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11898;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37103, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 22822;

-- Blood
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37104, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10275;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37105, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10281;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37106, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11905;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37107, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23244;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37104, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10275;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37105, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10281;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37106, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11905;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37107, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23244;

-- Guristas
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37108, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2382;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37109, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2387;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37110, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11932;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37111, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23321;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37108, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2382;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37109, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2387;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37110, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11932;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37111, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23321;

-- Serpentis
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37112, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2370;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37113, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2381;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37114, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11921;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37115, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23438;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37112, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2370;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37113, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2381;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37114, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11921;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37115, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23438;

-- Rogue Drones
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37116, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25636;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37117, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25632;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37118, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25648;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 37119, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25640;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37116, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25636;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37117, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25632;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37118, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25648;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 37119, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25640;

-- +migrate Down
DELETE FROM dgmTypeEffects WHERE typeID >= 37000 AND typeID < 37200;
DELETE FROM dgmTypeAttributes WHERE typeID >= 37000 AND typeID < 37200;
DELETE FROM invTypes WHERE typeID >= 37000 AND typeID < 37200;
