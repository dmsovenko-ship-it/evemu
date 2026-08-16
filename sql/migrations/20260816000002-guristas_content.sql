-- Guristas content: sentry batteries in existing anomalies, new Rally
-- Point/Port/Sanctum anomalies, DED 4/10 Scout Outpost rename + DED 7/10
-- Pith's Penal Complex and 10/10 The Maze. All types are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Guristas anomalies (roomID == dungeonID)
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hidden Hideaway (2041)
(2041,17596,383,3500,0,2000), (2041,17596,383,-3500,0,-2000), (2041,17597,383,0,0,4000),
-- Forsaken Hideaway (2042) / Forlorn Hideaway (2043)
(2042,17596,383,3500,0,2000), (2042,17597,383,-3500,0,2000), (2042,24767,383,0,0,-4000),
(2043,17597,383,3500,0,2000), (2043,17597,383,-3500,0,-2000), (2043,17598,383,0,0,4000), (2043,17611,383,0,0,-4000),
-- Refuge (2045)
(2045,17596,383,3500,0,2000), (2045,17596,383,-3500,0,-2000), (2045,17598,383,0,0,4000),
-- Den (2046)
(2046,17596,383,3500,0,2000), (2046,17597,383,-3500,0,2000), (2046,17611,383,0,0,-4000),
-- Hub (2047)
(2047,17598,383,4500,0,1500), (2047,17598,383,-4500,0,-1500), (2047,17599,383,0,0,4500),
(2047,17611,383,0,0,-4500), (2047,28141,383,-2000,0,4000),
-- Forlorn Hub (2048)
(2048,17598,383,4500,0,1500), (2048,17598,383,-4500,0,-1500), (2048,17598,383,0,0,5000),
(2048,17599,383,4500,0,-2000), (2048,17599,383,-4500,0,-2000), (2048,17611,383,0,0,-4500),
(2048,28146,383,2000,0,4500), (2048,28146,383,-2000,0,-4500);

-- =========================================================================
-- 2. Guristas Rally Point (2097) — frigate/destroyer waves + Dread faction
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2097;
DELETE FROM `dunRooms` WHERE `roomID` = 2097;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2097;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2097, 'Guristas Rally Point', 0, 500010, 7, 'guristas-rallypoint');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2097,'Rally Pocket',2097);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2097,17006,615,0,0,1500), (2097,17001,615,-1500,0,0), (2097,16981,615,1500,0,-750),
(2097,23971,614,0,0,-2500), (2097,23984,614,-2500,0,1500),
(2097,16980,613,2500,0,-1500), (2097,16991,613,0,0,3500),
(2097,13598,800,0,0,-4000),                       -- Dread Guristas Saboteur (faction spawn)
(2097,17596,383,3500,0,2000), (2097,17597,383,-3500,0,2000), (2097,17611,383,0,0,-4500);

-- =========================================================================
-- 3. Guristas Port (2098) — dockyard zone, destroyers + BCs
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2098;
DELETE FROM `dunRooms` WHERE `roomID` = 2098;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2098;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2098, 'Guristas Port', 0, 500010, 7, 'guristas-port');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2098,'Port Pocket',2098);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2098,23971,614,0,0,1500), (2098,23984,614,-1500,0,0), (2098,23985,614,1500,0,-750),
(2098,16980,613,0,0,-2500), (2098,16991,613,-2500,0,1500),
(2098,24001,611,2500,0,-1500), (2098,24014,611,0,0,3500),
(2098,17598,383,3500,0,2000), (2098,17596,383,-3500,0,2000), (2098,24767,383,0,0,-4500);

-- =========================================================================
-- 4. Guristas Sanctum (2099) — heavy: BCs + battleships + Dread elites
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2099;
DELETE FROM `dunRooms` WHERE `roomID` = 2099;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2099;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2099, 'Guristas Sanctum', 0, 500010, 7, 'guristas-sanctum');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2099,'Sanctum Pocket',2099);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2099,24001,611,0,0,1500), (2099,24015,611,-1500,0,0), (2099,24016,611,1500,0,-750),
(2099,16984,612,0,0,-2500), (2099,16988,612,-2500,0,1500), (2099,24145,612,2500,0,-1500),
(2099,13576,800,0,0,-4000),                       -- Dread Guristas Arrogator
(2099,13591,798,3000,0,3000),                     -- Dread Guristas Mortifier
(2099,17598,383,4500,0,2000), (2099,17599,383,-4500,0,2000), (2099,17611,383,0,0,-4500), (2099,28146,383,-2000,0,4000);

-- =========================================================================
-- 5. DED 4/10 renamed to Guristas Scout Outpost (2520) + sentries
-- =========================================================================
UPDATE `dunDungeons` SET `dungeonName` = 'Guristas Scout Outpost' WHERE `dungeonID` = 2520;
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2520,17596,383,3000,0,1500), (2520,13068,383,-3000,0,1500),
(2521,17597,383,3000,0,1500), (2521,17611,383,-3000,0,1500),
(2522,17598,383,3000,0,1500), (2522,17599,383,-3000,0,1500);

-- =========================================================================
-- 6. DED 7/10 — Pith's Penal Complex (2740), 4 rooms, Guristas Control Center boss
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2740 AND 2743;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2740 AND 2743;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2740;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2740, 'Pith''s Penal Complex', 0, 500010, 10, 'guristas-ded-7of10');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2740,'Penal Gate',2740), (2741,'Detention Block',2740), (2742,'Prison Yard',2740), (2743,'Control Center',2740);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2740,17006,615,0,0,1500), (2740,17001,615,-1500,0,0), (2740,16981,615,1500,0,-750),
(2740,13068,383,3000,0,1500), (2740,17596,383,-3000,0,1500),
(2740,2902,319,0,0,5000),
-- Room 2
(2741,23971,614,0,0,1500), (2741,23984,614,-1500,0,0), (2741,16980,613,1500,0,-750),
(2741,17597,383,3000,0,1500), (2741,17611,383,-3000,0,1500),
(2741,2902,319,0,0,5000),
-- Room 3
(2742,16980,613,0,0,1500), (2742,16991,613,-1500,0,0), (2742,24001,611,1500,0,-750),
(2742,17598,383,3000,0,1500), (2742,28141,383,-3000,0,1500),
(2742,2902,319,0,0,5000),
-- Room 4: Guristas Control Center + Dread Pith escort
(2743,26248,494,0,0,0),                  -- Guristas Control Center
(2743,24151,612,1500,0,-750), (2743,24153,612,-1500,0,0),
(2743,24187,611,0,0,3000),
(2743,17598,383,4000,0,1500), (2743,17599,383,-4000,0,1500), (2743,17611,383,0,0,-4000);

-- =========================================================================
-- 7. DED 10/10 — The Maze (2940), 5 rooms, Mazed Karadom's Armageddon boss
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2940 AND 2944;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2940 AND 2944;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2940;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2940, 'The Maze', 0, 500010, 10, 'guristas-ded-10of10');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2940,'Maze Gate',2940), (2941,'Maze Corridor',2940), (2942,'Maze Hall',2940),
(2943,'Maze Heart',2940), (2944,'Maze Core',2940);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2940,17006,615,0,0,1500), (2940,17001,615,-1500,0,0), (2940,16986,615,1500,0,-750),
(2940,17596,383,3000,0,1500), (2940,13068,383,-3000,0,1500),
(2940,2902,319,0,0,5000),
-- Room 2
(2941,23971,614,0,0,1500), (2941,23984,614,-1500,0,0), (2941,16980,613,1500,0,-750),
(2941,17597,383,3000,0,1500), (2941,17611,383,-3000,0,1500),
(2941,2902,319,0,0,5000),
-- Room 3
(2942,16980,613,0,0,1500), (2942,24001,611,-1500,0,0), (2942,24015,611,1500,0,-750),
(2942,17598,383,3000,0,1500), (2942,28141,383,-3000,0,1500),
(2942,2902,319,0,0,5000),
-- Room 4: battleships + Dread Pith
(2943,16984,612,0,0,1500), (2943,24145,612,-1500,0,0), (2943,17010,612,1500,0,-750),
(2943,17599,383,3000,0,1500), (2943,28146,383,-3000,0,1500),
(2943,2902,319,0,0,5000),
-- Room 5: Mazed Karadom's Armageddon + Dread Pith fleet
(2944,24664,517,0,0,0),                  -- Mazed Karadom's Armageddon (boss)
(2944,24151,612,1500,0,-750), (2944,24153,612,-1500,0,0), (2944,24187,611,0,0,3000),
(2944,17599,383,4000,0,1500), (2944,28146,383,-4000,0,1500), (2944,17611,383,0,0,-4000);

-- =========================================================================
-- 8. Guristas Hideaway (2040) rebuilt as wave-tiered pocket (removes stray
--    Renegade Angel Goon legacy rats), Burrow (2044) gets Pithi raider waves
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2040,2044);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hideaway: Pithi Wrecker/Plunderer screen + destroyers, light battery
(2040,17006,615,0,0,1500), (2040,17001,615,-1500,0,0), (2040,16981,615,1500,0,-750),
(2040,23971,614,0,0,-2500), (2040,23984,614,-2500,0,1500),
(2040,13068,383,3500,0,2000), (2040,17596,383,-3500,0,2000),
(2040,2902,319,0,0,5000),
-- Burrow: scout frigates + aggressive Pithi Plunderer/Wrecker, missile battery
(2044,16993,615,0,0,1500), (2044,16981,615,-1500,0,0), (2044,17006,615,1500,0,-750),
(2044,17001,615,0,0,-2500), (2044,17002,615,-2500,0,1500), (2044,16994,615,2500,0,-1500),
(2044,17596,383,3500,0,2000), (2044,17597,383,-3500,0,2000),
(2044,2902,319,0,0,5000);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2097,2098,2099) OR `roomID` BETWEEN 2740 AND 2743 OR `roomID` BETWEEN 2940 AND 2944;
DELETE FROM `dunRooms` WHERE `roomID` IN (2097,2098,2099) OR `roomID` BETWEEN 2740 AND 2743 OR `roomID` BETWEEN 2940 AND 2944;
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2097,2098,2099,2740,2940);
