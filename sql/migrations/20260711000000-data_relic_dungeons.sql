-- Radar (data) and Magnetometric (relic) exploration site dungeons
-- +migrate Up

-- ====== Magnetometric / Relic Sites (archetypeID=3) ======

-- Angel Cartel (factionID 500011)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2200, 'Angel Ruins', 3, 500011, 3, 'angel-rel-1'),
(2201, 'Angel Relic Site', 3, 500011, 3, 'angel-rel-2');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2200, 'Relic Pocket', 2200), (2201, 'Relic Pocket', 2201);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2200, 23,0, 0,0,0), (2200, 23,0, 200,0,150), (2200, 23,0, -200,0,-150),
(2201, 23,0, 0,0,0), (2201, 23,0, 150,0,100);

-- Blood Raiders (factionID 500012)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2210, 'Blood Raiders Ruins', 3, 500012, 3, 'blood-rel-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2210, 'Relic Pocket', 2210);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2210, 23,0, 0,0,0), (2210, 23,0, 180,0,120);

-- Guristas (factionID 500010)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2220, 'Guristas Ruins', 3, 500010, 3, 'guristas-rel-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2220, 'Relic Pocket', 2220);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2220, 23,0, 0,0,0), (2220, 23,0, -180,0,-120);

-- Sansha (factionID 500019)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2230, 'Sansha Ruins', 3, 500019, 3, 'sansha-rel-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2230, 'Relic Pocket', 2230);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2230, 23,0, 0,0,0), (2230, 23,0, 150,0,-100);

-- Serpentis (factionID 500013)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2240, 'Serpentis Ruins', 3, 500013, 3, 'serpentis-rel-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2240, 'Relic Pocket', 2240);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2240, 23,0, 0,0,0), (2240, 23,0, -150,0,100);

-- ====== Radar / Data Sites (archetypeID=4) ======

-- Angel Cartel (factionID 500011)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2250, 'Angel Data Site', 3, 500011, 4, 'angel-dat-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2250, 'Data Pocket', 2250);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2250, 23,0, 0,0,0), (2250, 23,0, 150,0,0);

-- Blood Raiders (factionID 500012)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2260, 'Blood Raiders Data Site', 3, 500012, 4, 'blood-dat-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2260, 'Data Pocket', 2260);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2260, 23,0, 0,0,0), (2260, 23,0, -150,0,0);

-- Guristas (factionID 500010)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2270, 'Guristas Data Site', 3, 500010, 4, 'guristas-dat-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2270, 'Data Pocket', 2270);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2270, 23,0, 0,0,0);

-- Sansha (factionID 500019)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2280, 'Sansha Data Site', 3, 500019, 4, 'sansha-dat-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2280, 'Data Pocket', 2280);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2280, 23,0, 0,0,0);

-- Serpentis (factionID 500013)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2290, 'Serpentis Data Site', 3, 500013, 4, 'serpentis-dat-1');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2290, 'Data Pocket', 2290);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2290, 23,0, 0,0,0);

-- +migrate Down
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2200 AND 2299;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2200 AND 2299;
