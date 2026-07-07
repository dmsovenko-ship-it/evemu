-- Replace SDE Entity typeIDs in dunRoomObjects with Ship-category clone typeIDs
-- +migrate Up

-- Angel: 2372->33504, 10017->33505, 11898->33506, 22822->33507
UPDATE dunRoomObjects SET typeID = 33504 WHERE typeID = 2372;
UPDATE dunRoomObjects SET typeID = 33505 WHERE typeID = 10017;
UPDATE dunRoomObjects SET typeID = 33506 WHERE typeID = 11898;
UPDATE dunRoomObjects SET typeID = 33507 WHERE typeID = 22822;

-- Blood: 10275->33508, 10281->33509, 11905->33510, 23244->33511
UPDATE dunRoomObjects SET typeID = 33508 WHERE typeID = 10275;
UPDATE dunRoomObjects SET typeID = 33509 WHERE typeID = 10281;
UPDATE dunRoomObjects SET typeID = 33510 WHERE typeID = 11905;
UPDATE dunRoomObjects SET typeID = 33511 WHERE typeID = 23244;

-- Guristas: 2382->33512, 2387->33513, 11932->33514, 23321->33515
UPDATE dunRoomObjects SET typeID = 33512 WHERE typeID = 2382;
UPDATE dunRoomObjects SET typeID = 33513 WHERE typeID = 2387;
UPDATE dunRoomObjects SET typeID = 33514 WHERE typeID = 11932;
UPDATE dunRoomObjects SET typeID = 33515 WHERE typeID = 23321;

-- Sansha: 10025->33500, 10030->33501, 11913->33502, 23383->33503
UPDATE dunRoomObjects SET typeID = 33500 WHERE typeID = 10025;
UPDATE dunRoomObjects SET typeID = 33501 WHERE typeID = 10030;
UPDATE dunRoomObjects SET typeID = 33502 WHERE typeID = 11913;
UPDATE dunRoomObjects SET typeID = 33503 WHERE typeID = 23383;

-- Serpentis: 2370->33516, 2381->33517, 11921->33518, 23438->33519
UPDATE dunRoomObjects SET typeID = 33516 WHERE typeID = 2370;
UPDATE dunRoomObjects SET typeID = 33517 WHERE typeID = 2381;
UPDATE dunRoomObjects SET typeID = 33518 WHERE typeID = 11921;
UPDATE dunRoomObjects SET typeID = 33519 WHERE typeID = 23438;

-- Rogue Drones: 25636->33520, 25632->33521, 25648->33522, 25640->33523
UPDATE dunRoomObjects SET typeID = 33520 WHERE typeID = 25636;
UPDATE dunRoomObjects SET typeID = 33521 WHERE typeID = 25632;
UPDATE dunRoomObjects SET typeID = 33522 WHERE typeID = 25648;
UPDATE dunRoomObjects SET typeID = 33523 WHERE typeID = 25640;

-- +migrate Down
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
