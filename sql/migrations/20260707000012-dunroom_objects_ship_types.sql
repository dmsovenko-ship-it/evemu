-- Replace SDE Entity typeIDs in dunRoomObjects with Ship-category clone typeIDs
-- This ensures all anomaly NPCs render with targeting crosshairs
-- +migrate Up

-- Angel Cartel: 2372→37100, 10017→37101, 11898→37102, 22822→37103
UPDATE dunRoomObjects SET typeID = 37100 WHERE typeID = 2372;
UPDATE dunRoomObjects SET typeID = 37101 WHERE typeID = 10017;
UPDATE dunRoomObjects SET typeID = 37102 WHERE typeID = 11898;
UPDATE dunRoomObjects SET typeID = 37103 WHERE typeID = 22822;

-- Blood Raiders: 10275→37104, 10281→37105, 11905→37106, 23244→37107
UPDATE dunRoomObjects SET typeID = 37104 WHERE typeID = 10275;
UPDATE dunRoomObjects SET typeID = 37105 WHERE typeID = 10281;
UPDATE dunRoomObjects SET typeID = 37106 WHERE typeID = 11905;
UPDATE dunRoomObjects SET typeID = 37107 WHERE typeID = 23244;

-- Guristas: 2382→37108, 2387→37109, 11932→37110, 23321→37111
UPDATE dunRoomObjects SET typeID = 37108 WHERE typeID = 2382;
UPDATE dunRoomObjects SET typeID = 37109 WHERE typeID = 2387;
UPDATE dunRoomObjects SET typeID = 37110 WHERE typeID = 11932;
UPDATE dunRoomObjects SET typeID = 37111 WHERE typeID = 23321;

-- Sansha: 10025→37000, 10030→37001, 11913→37002, 23383→37003
-- (includes incursion dungeon rooms 2100-2122)
UPDATE dunRoomObjects SET typeID = 37000 WHERE typeID = 10025;
UPDATE dunRoomObjects SET typeID = 37001 WHERE typeID = 10030;
UPDATE dunRoomObjects SET typeID = 37002 WHERE typeID = 11913;
UPDATE dunRoomObjects SET typeID = 37003 WHERE typeID = 23383;

-- Serpentis: 2370→37112, 2381→37113, 11921→37114, 23438→37115
UPDATE dunRoomObjects SET typeID = 37112 WHERE typeID = 2370;
UPDATE dunRoomObjects SET typeID = 37113 WHERE typeID = 2381;
UPDATE dunRoomObjects SET typeID = 37114 WHERE typeID = 11921;
UPDATE dunRoomObjects SET typeID = 37115 WHERE typeID = 23438;

-- Rogue Drones: 25636→37116, 25632→37117, 25648→37118, 25640→37119
UPDATE dunRoomObjects SET typeID = 37116 WHERE typeID = 25636;
UPDATE dunRoomObjects SET typeID = 37117 WHERE typeID = 25632;
UPDATE dunRoomObjects SET typeID = 37118 WHERE typeID = 25648;
UPDATE dunRoomObjects SET typeID = 37119 WHERE typeID = 25640;

-- +migrate Down
-- Restore original SDE typeIDs
UPDATE dunRoomObjects SET typeID = 2372  WHERE typeID = 37100;
UPDATE dunRoomObjects SET typeID = 10017 WHERE typeID = 37101;
UPDATE dunRoomObjects SET typeID = 11898 WHERE typeID = 37102;
UPDATE dunRoomObjects SET typeID = 22822 WHERE typeID = 37103;
UPDATE dunRoomObjects SET typeID = 10275 WHERE typeID = 37104;
UPDATE dunRoomObjects SET typeID = 10281 WHERE typeID = 37105;
UPDATE dunRoomObjects SET typeID = 11905 WHERE typeID = 37106;
UPDATE dunRoomObjects SET typeID = 23244 WHERE typeID = 37107;
UPDATE dunRoomObjects SET typeID = 2382  WHERE typeID = 37108;
UPDATE dunRoomObjects SET typeID = 2387  WHERE typeID = 37109;
UPDATE dunRoomObjects SET typeID = 11932 WHERE typeID = 37110;
UPDATE dunRoomObjects SET typeID = 23321 WHERE typeID = 37111;
UPDATE dunRoomObjects SET typeID = 37000 WHERE typeID = 10025;
UPDATE dunRoomObjects SET typeID = 37001 WHERE typeID = 10030;
UPDATE dunRoomObjects SET typeID = 37002 WHERE typeID = 11913;
UPDATE dunRoomObjects SET typeID = 37003 WHERE typeID = 23383;
UPDATE dunRoomObjects SET typeID = 2370  WHERE typeID = 37112;
UPDATE dunRoomObjects SET typeID = 2381  WHERE typeID = 37113;
UPDATE dunRoomObjects SET typeID = 11921 WHERE typeID = 37114;
UPDATE dunRoomObjects SET typeID = 23438 WHERE typeID = 37115;
UPDATE dunRoomObjects SET typeID = 25636 WHERE typeID = 37116;
UPDATE dunRoomObjects SET typeID = 25632 WHERE typeID = 37117;
UPDATE dunRoomObjects SET typeID = 25648 WHERE typeID = 37118;
UPDATE dunRoomObjects SET typeID = 25640 WHERE typeID = 37119;
