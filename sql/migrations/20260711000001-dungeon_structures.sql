-- +migrate Up
-- Add structures (sentry guns, acceleration gates, containers) to anomaly/incursion dungeons
--
-- Sentry guns (groupID 294):
--   14343 = Angel Sentry Gun      14344 = Blood Raider Sentry Gun
--   14345 = Guristas Sentry Gun   14346 = Sansha Sentry Gun
--   14347 = Serpentis Sentry Gun  22175 = Rogue Drone Sentry Gun
-- Acceleration gates (groupID 42):
--   23193 = Standard Acceleration Gate
-- Containers (groupID 9):
--   244 = Standard Cargo Container

-- ====== ANGEL (room 2000) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2000, 23193, 42, 30000, 0, 0),
(2000, 14343, 294, -1000, 800, 600),
(2000, 14343, 294, -1000, -800, -600);

-- ====== BLOOD RAIDER (room 2020) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2020, 23193, 42, 30000, 0, 0),
(2020, 14344, 294, -1000, 800, 600),
(2020, 14344, 294, -1000, -800, -600);

-- ====== GURISTAS (room 2040) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2040, 23193, 42, 30000, 0, 0),
(2040, 14345, 294, -1000, 800, 600),
(2040, 14345, 294, -1000, -800, -600);

-- ====== SANSHA INCURSION (room 2100) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2100, 23193, 42, 30000, 0, 0),
(2100, 14346, 294, -1000, 800, 600),
(2100, 14346, 294, -1000, -800, -600),
(2100, 244, 9, 0, 0, 0);

-- ====== SERPENTIS (room 2080) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2080, 23193, 42, 30000, 0, 0),
(2080, 14347, 294, -1000, 800, 600),
(2080, 14347, 294, -1000, -800, -600);

-- ====== ROGUE DRONES (room 2090) ======
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2090, 23193, 42, 30000, 0, 0),
(2090, 22175, 294, -1000, 800, 600),
(2090, 22175, 294, -1000, -800, -600);

-- +migrate Down
DELETE FROM `dunRoomObjects`
WHERE `typeID` IN (23193, 14343, 14344, 14345, 14346, 14347, 22175, 244)
  AND `roomID` IN (2000, 2020, 2040, 2080, 2090, 2100);
