-- DED-rated complexes 3/10 (Rated, archetypeID=10)
-- Each complex: 3 rooms with NPCs + containers. Final room has overseer + loot.
-- +migrate Up

-- ====== Angel Cartel 3/10 (dungeonID 2300-2310) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2300, 'Angel 3/10 DED Complex', 3, 500011, 10, 'angel-ded-310');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2300, 'Entry Pocket', 2300),
(2301, 'Second Pocket', 2300),
(2302, 'Command Pocket', 2300);

-- Room 2300: Entry - 3 frigates + 2 containers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2300, 23,0, 0,0,0), (2300, 23,0, 200,0,150),
(2300, 2372,0, -300,0,200), (2300, 10017,0, 300,0,-200), (2300, 2372,0, -100,0,-300);

-- Room 2301: Second - 2 cruisers + 1 frig + 3 containers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2301, 23,0, 0,0,0), (2301, 23,0, -200,0,100), (2301, 23,0, 200,0,-100),
(2301, 10017,0, -400,0,300), (2301, 11898,0, 400,0,-300), (2301, 2372,0, 0,0,400);

-- Room 2302: Command - 1 BC + 1 cruiser + 1 overseer + 2 loot containers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2302, 23,0, 0,0,0), (2302, 23,0, -150,0,150),
(2302, 22822,0, -400,0,300), (2302, 10017,0, 400,0,-300);

-- ====== Blood Raiders 3/10 (dungeonID 2310-2320) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2310, 'Blood 3/10 DED Complex', 3, 500012, 10, 'blood-ded-310');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2310, 'Entry Pocket', 2310), (2311, 'Second Pocket', 2310), (2312, 'Command Pocket', 2310);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2310, 23,0, 0,0,0), (2310, 23,0, 200,0,150),
(2310, 10275,0, -300,0,200), (2310, 10281,0, 300,0,-200), (2310, 10275,0, -100,0,-300),
(2311, 23,0, 0,0,0), (2311, 23,0, -200,0,100), (2311, 23,0, 200,0,-100),
(2311, 10281,0, -400,0,300), (2311, 11905,0, 400,0,-300), (2311, 10275,0, 0,0,400),
(2312, 23,0, 0,0,0), (2312, 23,0, -150,0,150),
(2312, 23244,0, -400,0,300), (2312, 10281,0, 400,0,-300);

-- ====== Guristas 3/10 (dungeonID 2320-2330) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2320, 'Guristas 3/10 DED Complex', 3, 500010, 10, 'guristas-ded-310');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2320, 'Entry Pocket', 2320), (2321, 'Second Pocket', 2320), (2322, 'Command Pocket', 2320);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2320, 23,0, 0,0,0), (2320, 23,0, 200,0,150),
(2320, 2382,0, -300,0,200), (2320, 2387,0, 300,0,-200), (2320, 2382,0, -100,0,-300),
(2321, 23,0, 0,0,0), (2321, 23,0, -200,0,100), (2321, 23,0, 200,0,-100),
(2321, 2387,0, -400,0,300), (2321, 11932,0, 400,0,-300), (2321, 2382,0, 0,0,400),
(2322, 23,0, 0,0,0), (2322, 23,0, -150,0,150),
(2322, 23321,0, -400,0,300), (2322, 2387,0, 400,0,-300);

-- ====== Sansha 3/10 (dungeonID 2330-2340) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2330, 'Sansha 3/10 DED Complex', 3, 500019, 10, 'sansha-ded-310');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2330, 'Entry Pocket', 2330), (2331, 'Second Pocket', 2330), (2332, 'Command Pocket', 2330);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2330, 23,0, 0,0,0), (2330, 23,0, 200,0,150),
(2330, 10025,0, -300,0,200), (2330, 10030,0, 300,0,-200), (2330, 10025,0, -100,0,-300),
(2331, 23,0, 0,0,0), (2331, 23,0, -200,0,100), (2331, 23,0, 200,0,-100),
(2331, 10030,0, -400,0,300), (2331, 11913,0, 400,0,-300), (2331, 10025,0, 0,0,400),
(2332, 23,0, 0,0,0), (2332, 23,0, -150,0,150),
(2332, 23383,0, -400,0,300), (2332, 10030,0, 400,0,-300);

-- ====== Serpentis 3/10 (dungeonID 2340-2350) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2340, 'Serpentis 3/10 DED Complex', 3, 500013, 10, 'serpentis-ded-310');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2340, 'Entry Pocket', 2340), (2341, 'Second Pocket', 2340), (2342, 'Command Pocket', 2340);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2340, 23,0, 0,0,0), (2340, 23,0, 200,0,150),
(2340, 2370,0, -300,0,200), (2340, 2381,0, 300,0,-200), (2340, 2370,0, -100,0,-300),
(2341, 23,0, 0,0,0), (2341, 23,0, -200,0,100), (2341, 23,0, 200,0,-100),
(2341, 2381,0, -400,0,300), (2341, 11921,0, 400,0,-300), (2341, 2370,0, 0,0,400),
(2342, 23,0, 0,0,0), (2342, 23,0, -150,0,150),
(2342, 23438,0, -400,0,300), (2342, 2381,0, 400,0,-300);

-- ====== Rogue Drones 3/10 (dungeonID 2350-2360) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2350, 'Rogue Drone 3/10 DED Complex', 3, 500022, 10, 'drone-ded-310');
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2350, 'Entry Pocket', 2350), (2351, 'Second Pocket', 2350), (2352, 'Command Pocket', 2350);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2350, 23,0, 0,0,0), (2350, 23,0, 200,0,150),
(2350, 25636,0, -300,0,200), (2350, 25632,0, 300,0,-200), (2350, 25636,0, -100,0,-300),
(2351, 23,0, 0,0,0), (2351, 23,0, -200,0,100), (2351, 23,0, 200,0,-100),
(2351, 25632,0, -400,0,300), (2351, 25648,0, 400,0,-300), (2351, 25636,0, 0,0,400),
(2352, 23,0, 0,0,0), (2352, 23,0, -150,0,150),
(2352, 25640,0, -400,0,300), (2352, 25632,0, 400,0,-300);

-- +migrate Down
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2300 AND 2359;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2300 AND 2359;
