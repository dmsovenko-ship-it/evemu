-- Revert migration 00012: restore original SDE typeIDs in dunRoomObjects
-- Run: mysql -h db -u evemu -pevemu evemu < revert_dunroom_objects.sql

-- Sansha: 33500-33503 -> 10025/10030/11913/23383
UPDATE dunRoomObjects SET typeID = 10025 WHERE typeID = 33500;
UPDATE dunRoomObjects SET typeID = 10030 WHERE typeID = 33501;
UPDATE dunRoomObjects SET typeID = 11913 WHERE typeID = 33502;
UPDATE dunRoomObjects SET typeID = 23383 WHERE typeID = 33503;

-- Angel: 33504-33507 -> 2372/10017/11898/22822
UPDATE dunRoomObjects SET typeID = 2372  WHERE typeID = 33504;
UPDATE dunRoomObjects SET typeID = 10017 WHERE typeID = 33505;
UPDATE dunRoomObjects SET typeID = 11898 WHERE typeID = 33506;
UPDATE dunRoomObjects SET typeID = 22822 WHERE typeID = 33507;

-- Blood: 33508-33511 -> 10275/10281/11905/23244
UPDATE dunRoomObjects SET typeID = 10275 WHERE typeID = 33508;
UPDATE dunRoomObjects SET typeID = 10281 WHERE typeID = 33509;
UPDATE dunRoomObjects SET typeID = 11905 WHERE typeID = 33510;
UPDATE dunRoomObjects SET typeID = 23244 WHERE typeID = 33511;

-- Guristas: 33512-33515 -> 2382/2387/11932/23321
UPDATE dunRoomObjects SET typeID = 2382  WHERE typeID = 33512;
UPDATE dunRoomObjects SET typeID = 2387  WHERE typeID = 33513;
UPDATE dunRoomObjects SET typeID = 11932 WHERE typeID = 33514;
UPDATE dunRoomObjects SET typeID = 23321 WHERE typeID = 33515;

-- Serpentis: 33516-33519 -> 2370/2381/11921/23438
UPDATE dunRoomObjects SET typeID = 2370  WHERE typeID = 33516;
UPDATE dunRoomObjects SET typeID = 2381  WHERE typeID = 33517;
UPDATE dunRoomObjects SET typeID = 11921 WHERE typeID = 33518;
UPDATE dunRoomObjects SET typeID = 23438 WHERE typeID = 33519;

-- Rogue Drones: 33520-33523 -> 25636/25632/25648/25640
UPDATE dunRoomObjects SET typeID = 25636 WHERE typeID = 33520;
UPDATE dunRoomObjects SET typeID = 25632 WHERE typeID = 33521;
UPDATE dunRoomObjects SET typeID = 25648 WHERE typeID = 33522;
UPDATE dunRoomObjects SET typeID = 25640 WHERE typeID = 33523;

-- Remove orphan 33500-33523 types and their dogma data (from migration 00011)
DELETE FROM dgmTypeEffects WHERE typeID >= 33500 AND typeID < 33524;
DELETE FROM dgmTypeAttributes WHERE typeID >= 33500 AND typeID < 33524;
DELETE FROM invTypesToWrecks WHERE typeID >= 33500 AND typeID < 33524;
DELETE FROM invTypes WHERE typeID >= 33500 AND typeID < 33524;

SELECT CONCAT('Fixed ', COUNT(*), ' rows in dunRoomObjects') AS status FROM dunRoomObjects WHERE typeID IN (10025,10030,11913,23383,2372,10017,11898,22822,10275,10281,11905,23244,2382,2387,11932,23321,2370,2381,11921,23438,25636,25632,25648,25640);
