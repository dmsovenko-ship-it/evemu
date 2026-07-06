-- Incursion site dungeon definitions for Vanguard, Assault, Headquarters
-- Uses Incursion Sansha NPC groupIDs (1053=Frigate, 1054=Cruiser, 1056=Battleship)
-- +migrate Up

-- Vanguard sites (dungeonID 2100-2109)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2100, 'Incursion Vanguard - Nation Mining Colony', 3, 500019, 7, 'inc-vg-mining'),
(2101, 'Incursion Vanguard - Nation Trading Post', 3, 500019, 7, 'inc-vg-trading'),
(2102, 'Incursion Vanguard - Nation Supply Station', 3, 500019, 7, 'inc-vg-supply'),
(2103, 'Incursion Vanguard - Nation Communications', 3, 500019, 7, 'inc-vg-comm');

-- Assault sites (dungeonID 2110-2114)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2110, 'Incursion Assault - Nation Shipyard', 3, 500019, 7, 'inc-as-shipyard'),
(2111, 'Incursion Assault - Nation Weapon Factory', 3, 500019, 7, 'inc-as-weapon'),
(2112, 'Incursion Assault - Nation Command Center', 3, 500019, 7, 'inc-as-command');

-- Headquarters sites (dungeonID 2120-2122)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2120, 'Incursion Headquarters - Nation Computer Center', 3, 500019, 7, 'inc-hq-computer'),
(2121, 'Incursion Headquarters - Nation Energy Grid', 3, 500019, 7, 'inc-hq-energy'),
(2122, 'Incursion Headquarters - Nation Core Assembly', 3, 500019, 7, 'inc-hq-core');

-- Rooms for each dungeon
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2100, 'VG Pocket', 2100), (2101, 'VG Pocket', 2101), (2102, 'VG Pocket', 2102), (2103, 'VG Pocket', 2103),
(2110, 'AS Pocket', 2110), (2111, 'AS Pocket', 2111), (2112, 'AS Pocket', 2112),
(2120, 'HQ Pocket', 2120), (2121, 'HQ Pocket', 2121), (2122, 'HQ Pocket', 2122);

-- Vanguard spawns: frigates + cruisers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2100,10025,0,0,0,1500),(2100,10030,0,1500,0,-750),(2100,10025,0,-1500,0,0),(2100,10030,0,0,0,-3000),
(2101,10025,0,0,0,1500),(2101,10030,0,1500,0,-750),(2101,11913,0,-1500,0,0),(2101,10025,0,0,0,-3000),(2101,10030,0,2500,0,0),
(2102,10025,0,0,0,1500),(2102,10030,0,1500,0,-750),(2102,11913,0,-1500,0,0),(2102,10030,0,0,0,-3000),(2102,10025,0,2500,0,0),(2102,11913,0,-2500,0,0),
(2103,10030,0,0,0,1500),(2103,11913,0,1500,0,-750),(2103,10025,0,-1500,0,0),(2103,11913,0,0,0,-3000),(2103,10030,0,2500,0,0),(2103,11913,0,-2500,0,0);

-- Assault spawns: cruisers + battleships
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2110,10030,0,0,0,1500),(2110,11913,0,1500,0,-750),(2110,10030,0,-1500,0,0),(2110,11913,0,0,0,-3000),(2110,23383,0,2500,0,0),
(2111,10030,0,0,0,1500),(2111,11913,0,1500,0,-750),(2111,23383,0,-1500,0,0),(2111,11913,0,0,0,-3000),(2111,10030,0,2500,0,0),(2111,23383,0,-2500,0,0),
(2112,11913,0,0,0,1500),(2112,23383,0,1500,0,-750),(2112,11913,0,-1500,0,0),(2112,23383,0,0,0,-3000),(2112,10030,0,2500,0,0),(2112,11913,0,-2500,0,0),(2112,23383,0,0,3000,0);

-- Headquarters spawns: battleships + battlecruisers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2120,11913,0,0,0,1500),(2120,23383,0,1500,0,-750),(2120,11913,0,-1500,0,0),(2120,23383,0,0,0,-3000),(2120,11913,0,2500,0,0),(2120,23383,0,-2500,0,0),
(2121,11913,0,0,0,1500),(2121,23383,0,1500,0,-750),(2121,11913,0,-1500,0,0),(2121,23383,0,0,0,-3000),(2121,11913,0,2500,0,0),(2121,23383,0,-2500,0,0),(2121,11913,0,0,3000,0),
(2122,11913,0,0,0,1500),(2122,23383,0,1500,0,-750),(2122,23383,0,-1500,0,0),(2122,11913,0,0,0,-3000),(2122,23383,0,2500,0,0),(2122,11913,0,-2500,0,0),(2122,23383,0,0,3000,0),(2122,11913,0,0,-3000,0);

-- Seed incursion rewards
INSERT IGNORE INTO incursionRewards (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(192, 0, 0, 0, 0); -- default Sansha incursion (placeholder)

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2100 AND 2122;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2100 AND 2122;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2100 AND 2122;
