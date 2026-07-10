-- Revert custom Ship-category typeIDs back to SDE Entity-category typeIDs in dunRoomObjects.
-- The custom 34000-34023 typeIDs remain in invTypes but aren't used for spawning.
-- MakeSlimItem() + Generate_invTypes() handle the Ship-category remap at runtime.
-- +migrate Up

-- Anomaly dungeons (rooms 2000-2093): revert from 34000-34023 to SDE typeIDs
UPDATE dunRoomObjects SET typeID = 2372 WHERE typeID = 34000;
UPDATE dunRoomObjects SET typeID = 10017 WHERE typeID = 34001;
UPDATE dunRoomObjects SET typeID = 11898 WHERE typeID = 34002;
UPDATE dunRoomObjects SET typeID = 22822 WHERE typeID = 34003;
UPDATE dunRoomObjects SET typeID = 10275 WHERE typeID = 34004;
UPDATE dunRoomObjects SET typeID = 10281 WHERE typeID = 34005;
UPDATE dunRoomObjects SET typeID = 11905 WHERE typeID = 34006;
UPDATE dunRoomObjects SET typeID = 23244 WHERE typeID = 34007;
UPDATE dunRoomObjects SET typeID = 2382 WHERE typeID = 34008;
UPDATE dunRoomObjects SET typeID = 2387 WHERE typeID = 34009;
UPDATE dunRoomObjects SET typeID = 11932 WHERE typeID = 34010;
UPDATE dunRoomObjects SET typeID = 23321 WHERE typeID = 34011;
UPDATE dunRoomObjects SET typeID = 10025 WHERE typeID = 34012;
UPDATE dunRoomObjects SET typeID = 10030 WHERE typeID = 34013;
UPDATE dunRoomObjects SET typeID = 11913 WHERE typeID = 34014;
UPDATE dunRoomObjects SET typeID = 23383 WHERE typeID = 34015;
UPDATE dunRoomObjects SET typeID = 2370 WHERE typeID = 34016;
UPDATE dunRoomObjects SET typeID = 2381 WHERE typeID = 34017;
UPDATE dunRoomObjects SET typeID = 11921 WHERE typeID = 34018;
UPDATE dunRoomObjects SET typeID = 23438 WHERE typeID = 34019;
UPDATE dunRoomObjects SET typeID = 25636 WHERE typeID = 34020;
UPDATE dunRoomObjects SET typeID = 25632 WHERE typeID = 34021;
UPDATE dunRoomObjects SET typeID = 25648 WHERE typeID = 34022;
UPDATE dunRoomObjects SET typeID = 25640 WHERE typeID = 34023;

-- +migrate Down
-- (no revert — these are just runtime fixes, 34000-34023 stay in invTypes for future use)
