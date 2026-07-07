-- Revert: use original SDE Entity typeIDs in dunRoomObjects
-- Custom Ship-category typeIDs (33500+) cannot be loaded by Crucible client
-- +migrate Up

UPDATE dunRoomObjects SET typeID = 2372  WHERE typeID = 33504;
UPDATE dunRoomObjects SET typeID = 10017 WHERE typeID = 33505;
UPDATE dunRoomObjects SET typeID = 11898 WHERE typeID = 33506;
UPDATE dunRoomObjects SET typeID = 22822 WHERE typeID = 33507;
UPDATE dunRoomObjects SET typeID = 10275 WHERE typeID = 33508;
UPDATE dunRoomObjects SET typeID = 10281 WHERE typeID = 33509;
UPDATE dunRoomObjects SET typeID = 11905 WHERE typeID = 33510;
UPDATE dunRoomObjects SET typeID = 23244 WHERE typeID = 33511;
UPDATE dunRoomObjects SET typeID = 2382  WHERE typeID = 33512;
UPDATE dunRoomObjects SET typeID = 2387  WHERE typeID = 33513;
UPDATE dunRoomObjects SET typeID = 11932 WHERE typeID = 33514;
UPDATE dunRoomObjects SET typeID = 23321 WHERE typeID = 33515;
UPDATE dunRoomObjects SET typeID = 10025 WHERE typeID = 33500;
UPDATE dunRoomObjects SET typeID = 10030 WHERE typeID = 33501;
UPDATE dunRoomObjects SET typeID = 11913 WHERE typeID = 33502;
UPDATE dunRoomObjects SET typeID = 23383 WHERE typeID = 33503;
UPDATE dunRoomObjects SET typeID = 2370  WHERE typeID = 33516;
UPDATE dunRoomObjects SET typeID = 2381  WHERE typeID = 33517;
UPDATE dunRoomObjects SET typeID = 11921 WHERE typeID = 33518;
UPDATE dunRoomObjects SET typeID = 23438 WHERE typeID = 33519;
UPDATE dunRoomObjects SET typeID = 25636 WHERE typeID = 33520;
UPDATE dunRoomObjects SET typeID = 25632 WHERE typeID = 33521;
UPDATE dunRoomObjects SET typeID = 25648 WHERE typeID = 33522;
UPDATE dunRoomObjects SET typeID = 25640 WHERE typeID = 33523;

-- +migrate Down
UPDATE dunRoomObjects SET typeID = 33504 WHERE typeID = 2372;
UPDATE dunRoomObjects SET typeID = 33505 WHERE typeID = 10017;
-- etc (up for re-apply)
