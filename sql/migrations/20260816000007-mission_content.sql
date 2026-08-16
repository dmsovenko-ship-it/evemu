-- Mission content: dungeons for Vengeance / Recon (1 of 3) / Angel Sound arc
-- key missions, linked via qstEncounter so accepting the agent mission spawns
-- the pocket. All NPC types are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Mission: Vengeance (5100) — 3 pockets, Hoborak Moon boss + Angel
--    Battlestation backdrop (per lore: webfifiers first, battlestation is
--    scenery that only drops junk and never needs to be destroyed)
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 5100 AND 5102;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 5100 AND 5102;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 5100;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(5100, 'Vengeance', 0, 500011, 1, 'mission-vengeance');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(5100,'Vengeance Pocket 1',5100), (5101,'Vengeance Pocket 2',5100), (5102,'Vengeance Pocket 3',5100);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Pocket 1: scattered Angel groups 10-30km
(5100,2372,552,10000,0,5000), (5100,2372,552,-12000,0,8000), (5100,10017,553,15000,0,-5000),
(5100,10017,553,-18000,0,-10000), (5100,2372,552,22000,0,15000), (5100,16562,550,-25000,0,12000),
(5100,2902,319,0,0,30000),
-- Pocket 2: four groups up to 35km
(5101,2372,552,15000,0,8000), (5101,10017,553,25000,0,-8000), (5101,2372,552,-20000,0,12000), (5101,16562,550,-30000,0,-15000),
(5101,2902,319,0,0,35000),
-- Pocket 3: Hoborak Moon boss at 2km + webfifiers + Angel Battlestation scenery
(5102,20439,816,0,0,0),                      -- Hoborak Moon
(5102,16562,550,1500,0,500), (5102,16562,550,-1500,0,500),
(5102,10017,553,3000,0,-1500), (5102,10017,553,-3000,0,-1500),
(5102,2372,552,2500,0,2500), (5102,2372,552,-2500,0,2500),
(5102,11077,319,0,0,-8000);                  -- Angel Battlestation (scenery, LCS 319 -> celestial)

-- =========================================================================
-- 2. Recon (1 of 3) (5110) — Blood Raider territory, dockyards + waves,
--    key objective is reaching the acceleration gate (~24km)
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 5110 AND 5111;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 5110 AND 5111;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 5110;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(5110, 'Recon (1 of 3)', 0, 500012, 1, 'mission-recon1');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(5110,'Recon Pocket 1',5110), (5111,'Recon Pocket 2',5110);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Pocket 1: Blood Raiders dockyard, waves of Corpior/Corpum
(5110,10275,558,0,0,1500), (5110,10275,558,-1500,0,0), (5110,10281,555,1500,0,-750),
(5110,23970,605,0,0,-2500), (5110,23979,605,-2500,0,1500),
(5110,10281,555,2500,0,-1500), (5110,10281,555,0,0,3500),
(5110,2902,319,0,0,24000),
-- Pocket 2: heavier resistance, gate to objective
(5111,10281,555,0,0,1500), (5111,10281,555,-1500,0,0), (5111,11905,557,1500,0,-750),
(5111,11905,557,0,0,-3000), (5111,16944,604,-2500,0,1500),
(5111,2902,319,0,0,24000);

-- =========================================================================
-- 3. Angel Sound arc: Ride to the Rescue (5120) — hunt Yukiro Demense
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 5120;
DELETE FROM `dunRooms` WHERE `roomID` = 5120;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 5120;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(5120, 'Ride to the Rescue', 0, 500011, 1, 'arc-angelsound-ridetorescue');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(5120,'Ride Pocket',5120);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(5120,32391,615,0,0,0),                       -- Yukiro Demense
(5120,2372,552,1500,0,-750), (5120,10017,553,-1500,0,0),
(5120,10017,553,2500,0,-1500), (5120,16562,550,-2500,0,1500),
(5120,11077,319,0,0,-8000);                   -- Angel Battlestation scenery

-- =========================================================================
-- 4. Angel Sound arc: Fear of Angels (5130) — final fight
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 5130;
DELETE FROM `dunRooms` WHERE `roomID` = 5130;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 5130;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(5130, 'Fear of Angels', 0, 500011, 1, 'arc-angelsound-fearofangels');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(5130,'Fear Pocket',5130);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(5130,20439,816,0,0,0),                       -- Hoborak Moon-class final target
(5130,16562,550,1500,0,500), (5130,16562,550,-1500,0,500),
(5130,10017,553,3000,0,-1500), (5130,10017,553,-3000,0,-1500),
(5130,2372,552,2500,0,2500), (5130,11077,319,0,0,-8000);

-- =========================================================================
-- 5. Link missions to dungeons via qstEncounter
-- =========================================================================
INSERT IGNORE INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(54351, 0, 'Vengeance', 3, 8, 1, 0, 0, 0, 5100, 100000, 0, 0, 50000, 0),
(54352, 0, 'Vengeance', 3, 8, 1, 0, 0, 0, 5100, 100000, 0, 0, 50000, 0),
(54581, 0, 'Vengeance', 4, 8, 1, 0, 0, 0, 5100, 200000, 0, 0, 100000, 0),
(55369, 0, 'Recon (1 of 3)', 4, 8, 1, 0, 0, 0, 5110, 150000, 0, 0, 75000, 0),
(55420, 0, 'Recon (1 of 3)', 4, 8, 1, 0, 0, 0, 5110, 150000, 0, 0, 75000, 0),
(80064, 0, 'Ride to the Rescue', 3, 8, 1, 0, 0, 0, 5120, 350000, 0, 0, 175000, 0),
(80076, 0, 'Fear of Angels', 3, 8, 1, 0, 0, 0, 5130, 1000000, 0, 0, 500000, 0);

-- +migrate Down
DELETE FROM `qstEncounter` WHERE `id` IN (54351,54352,54581,55369,55420,80064,80076);
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 5100 AND 5102 OR `roomID` IN (5110,5111,5120,5130);
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 5100 AND 5102 OR `roomID` IN (5110,5111,5120,5130);
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (5100,5110,5120,5130);
