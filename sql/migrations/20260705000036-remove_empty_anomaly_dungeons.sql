-- Remove anomaly dungeon containers (migration 32) and NPC objects (migration 33/35)
-- DungeonMgr::MakeDungeon creates celestials for anomaly dungeons
-- These celestials get included in DoDestinyUpdate and can crash on invalid data
-- Keeping only AnomalyMgr naming changes (code, not SQL)
-- +migrate Up

DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2000 AND 2093;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2000 AND 2093;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2000 AND 2093;

-- +migrate Down
-- Re-add from migrations 20260705000032 + 20260705000033/35 if needed
