-- Ship-category NPC types (fixes crosshair rendering)
-- Client's built-in SDE has Entity NPCs as cat 11 -> no crosshairs
-- New Ship-category typeIDs unknown to client -> loaded from server cache
-- Using 33500-33523 range (within Crucible client limits)
-- +migrate Up

-- Sansha (33500-33503, src: 10025/10030/11913/23383)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33500, 25, 'Sansha Frigate', '', 1238, 50, 2112000, 21120, 100, 1, 16, 0, 0, 31),
(33501, 26, 'Sansha Cruiser', '', 1236, 150, 10010000, 100100, 200, 1, 16, 0, 0, 31),
(33502, 27, 'Sansha Battleship', '', 2295, 350, 10010000, 100100, 235, 1, 16, 0, 0, 31),
(33503, 419, 'Sansha Battlecruiser', '', 1236, 150, 10010000, 100100, 235, 1, 16, 0, 0, 31);

-- Angel (33504-33507)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33504, 25, 'Angel Frigate', '', 344, 50, 2250000, 22500, 75, 1, 4, 0, 0, 31),
(33505, 26, 'Angel Cruiser', '', 336, 150, 9900000, 99000, 1900, 1, 4, 0, 0, 31),
(33506, 27, 'Angel Battleship', '', 335, 350, 19000000, 19000000, 120, 1, 4, 0, 0, 31),
(33507, 419, 'Angel Battlecruiser', '', 26378, 150, 10900000, 109000, 120, 1, 4, 0, 0, 31);

-- Blood (33508-33511)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33508, 25, 'Blood Frigate', '', 1762, 50, 2810000, 28100, 120, 1, 8, 0, 0, 31),
(33509, 26, 'Blood Cruiser', '', 1760, 150, 11500000, 115000, 465, 1, 8, 0, 0, 31),
(33510, 27, 'Blood Battleship', '', 2122, 350, 20500000, 1100000, 235, 1, 8, 0, 0, 31),
(33511, 419, 'Blood Battlecruiser', '', 2795, 150, 11800000, 118000, 235, 1, 8, 0, 0, 31);

-- Guristas (33512-33515)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33512, 25, 'Guristas Frigate', '', 1968, 50, 1500100, 15001, 45, 1, 1, 0, 0, 31),
(33513, 26, 'Guristas Cruiser', '', 1823, 150, 10700000, 107000, 850, 1, 1, 0, 0, 31),
(33514, 27, 'Guristas Battleship', '', 2159, 350, 21000000, 1040000, 235, 1, 1, 0, 0, 31),
(33515, 419, 'Guristas Battlecruiser', '', 26204, 150, 10100000, 101000, 235, 1, 1, 0, 0, 31);

-- Serpentis (33516-33519)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33516, 25, 'Serpentis Frigate', '', 1816, 50, 2450000, 24500, 60, 1, 2, 0, 0, 31),
(33517, 26, 'Serpentis Cruiser', '', 1812, 150, 11600000, 116000, 900, 1, 2, 0, 0, 31),
(33518, 27, 'Serpentis Battleship', '', 2156, 350, 19000000, 1010000, 480, 1, 2, 0, 0, 31),
(33519, 419, 'Serpentis Battlecruiser', '', 2917, 150, 11200000, 112000, 480, 1, 2, 0, 0, 31);

-- Rogue Drones (33520-33523)
INSERT IGNORE INTO invTypes (typeID, groupID, typeName, description, graphicID, radius, mass, volume, capacity, portionSize, raceID, basePrice, published, soundID) VALUES
(33520, 25, 'Drone Frigate', '', 1226, 45, 100000, 60, 1200, 1, 0, 0, 0, 0),
(33521, 26, 'Drone Cruiser', '', 1222, 150, 100000, 60, 1200, 1, 0, 0, 0, 0),
(33522, 27, 'Drone Battleship', '', 1718, 350, 21000000, 1010000, 950, 1, 0, 0, 0, 0),
(33523, 419, 'Drone Battlecruiser', '', 1223, 150, 100000, 60, 1200, 1, 0, 0, 0, 0);

-- Copy dogma attributes and effects
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33500, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33501, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33502, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33503, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23383;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33500, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10025;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33501, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10030;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33502, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11913;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33503, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23383;

INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33504, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2372;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33505, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10017;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33506, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11898;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33507, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 22822;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33504, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2372;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33505, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10017;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33506, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11898;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33507, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 22822;

INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33508, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10275;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33509, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 10281;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33510, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11905;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33511, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23244;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33508, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10275;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33509, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 10281;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33510, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11905;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33511, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23244;

INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33512, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2382;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33513, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2387;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33514, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11932;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33515, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23321;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33512, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2382;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33513, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2387;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33514, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11932;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33515, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23321;

INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33516, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2370;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33517, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 2381;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33518, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 11921;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33519, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 23438;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33516, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2370;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33517, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 2381;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33518, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 11921;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33519, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 23438;

INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33520, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25636;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33521, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25632;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33522, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25648;
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt, valueFloat) SELECT 33523, attributeID, valueInt, valueFloat FROM dgmTypeAttributes WHERE typeID = 25640;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33520, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25636;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33521, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25632;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33522, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25648;
INSERT IGNORE INTO dgmTypeEffects (typeID, effectID, isDefault) SELECT 33523, effectID, isDefault FROM dgmTypeEffects WHERE typeID = 25640;

-- +migrate Down
DELETE FROM dgmTypeEffects WHERE typeID >= 33500 AND typeID < 33524;
DELETE FROM dgmTypeAttributes WHERE typeID >= 33500 AND typeID < 33524;
DELETE FROM invTypes WHERE typeID >= 33500 AND typeID < 33524;
