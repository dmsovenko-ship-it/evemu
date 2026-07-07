-- Wreck mappings for Ship-category NPC types
-- +migrate Up

-- Sansha wrecks: same wreck as original Entity types
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33500, wreckTypeID FROM invTypesToWrecks WHERE typeID = 10025;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33501, wreckTypeID FROM invTypesToWrecks WHERE typeID = 10030;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33502, wreckTypeID FROM invTypesToWrecks WHERE typeID = 11913;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33503, wreckTypeID FROM invTypesToWrecks WHERE typeID = 23383;
-- Angel
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33504, wreckTypeID FROM invTypesToWrecks WHERE typeID = 2372;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33505, wreckTypeID FROM invTypesToWrecks WHERE typeID = 10017;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33506, wreckTypeID FROM invTypesToWrecks WHERE typeID = 11898;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33507, wreckTypeID FROM invTypesToWrecks WHERE typeID = 22822;
-- Blood
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33508, wreckTypeID FROM invTypesToWrecks WHERE typeID = 10275;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33509, wreckTypeID FROM invTypesToWrecks WHERE typeID = 10281;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33510, wreckTypeID FROM invTypesToWrecks WHERE typeID = 11905;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33511, wreckTypeID FROM invTypesToWrecks WHERE typeID = 23244;
-- Guristas
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33512, wreckTypeID FROM invTypesToWrecks WHERE typeID = 2382;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33513, wreckTypeID FROM invTypesToWrecks WHERE typeID = 2387;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33514, wreckTypeID FROM invTypesToWrecks WHERE typeID = 11932;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33515, wreckTypeID FROM invTypesToWrecks WHERE typeID = 23321;
-- Serpentis
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33516, wreckTypeID FROM invTypesToWrecks WHERE typeID = 2370;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33517, wreckTypeID FROM invTypesToWrecks WHERE typeID = 2381;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33518, wreckTypeID FROM invTypesToWrecks WHERE typeID = 11921;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33519, wreckTypeID FROM invTypesToWrecks WHERE typeID = 23438;
-- Rogue Drones
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33520, wreckTypeID FROM invTypesToWrecks WHERE typeID = 25636;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33521, wreckTypeID FROM invTypesToWrecks WHERE typeID = 25632;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33522, wreckTypeID FROM invTypesToWrecks WHERE typeID = 25648;
INSERT IGNORE INTO invTypesToWrecks (typeID, wreckTypeID) SELECT 33523, wreckTypeID FROM invTypesToWrecks WHERE typeID = 25640;

-- +migrate Down
DELETE FROM invTypesToWrecks WHERE typeID >= 33500 AND typeID < 33524;
