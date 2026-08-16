-- Angel Cartel content: sentry batteries in existing anomalies, new Sanctum,
-- DED 3/10 Repurposed Outpost, Domination faction spawns, wave-tiered pockets.
-- All types are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Angel anomalies (roomID == dungeonID)
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hideaway (2000) / Hidden Hideaway (2001) / Forlorn Hideaway (2002)
(2001,13114,383,3500,0,2000), (2001,17572,383,-3500,0,2000),
(2002,17572,383,3500,0,2000), (2002,17573,383,-3500,0,-2000), (2002,17574,383,0,0,4000),
-- Refuge (2004) / Den (2005)
(2004,17572,383,3500,0,2000), (2004,17572,383,-3500,0,-2000), (2004,17573,383,0,0,4000),
(2005,17572,383,3500,0,2000), (2005,17573,383,-3500,0,2000), (2005,17605,383,0,0,-4000),
-- Hidden Rally Point (2006) / Forsaken Rally Point (2007)
(2006,17573,383,3500,0,2000), (2006,17572,383,-3500,0,-2000), (2006,17605,383,0,0,4000),
(2007,17573,383,3500,0,2000), (2007,17574,383,-3500,0,-2000), (2007,27280,383,0,0,4000),
-- Port (2008)
(2008,17572,383,3500,0,2000), (2008,17574,383,-3500,0,2000), (2008,17605,383,0,0,-4000),
-- Hub (2009) / Hidden Hub (2010)
(2009,17574,383,4500,0,1500), (2009,17574,383,-4500,0,-1500), (2009,17575,383,0,0,4500), (2009,17605,383,0,0,-4500),
(2010,17574,383,4500,0,1500), (2010,17575,383,-4500,0,-1500), (2010,17605,383,0,0,4500), (2010,28139,383,-2000,0,4000),
-- Haven (2011)
(2011,17574,383,4500,0,1500), (2011,17575,383,-4500,0,-1500), (2011,17575,383,0,0,5000),
(2011,17605,383,0,0,-4500), (2011,28144,383,-2000,0,4000);

-- =========================================================================
-- 2. Angel Sanctum (2012) — heavy: Gistatis BCs + Gist battleships + Domination
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2012;
DELETE FROM `dunRooms` WHERE `roomID` = 2012;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2012;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2012, 'Angel Sanctum', 0, 500011, 7, 'angel-sanctum');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2012,'Sanctum Pocket',2012);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2012,24175,593,0,0,1500), (2012,24176,593,-1500,0,0), (2012,24178,593,1500,0,-750),
(2012,16921,594,0,0,-2500), (2012,16922,594,-2500,0,1500), (2012,16926,594,2500,0,-1500),
(2012,13535,848,0,0,-4000),                       -- Domination Commander
(2012,13540,848,3000,0,3000),                     -- Domination Saint
(2012,17574,383,4500,0,2000), (2012,17575,383,-4500,0,2000), (2012,17605,383,0,0,-4500), (2012,28144,383,-2000,0,4000);

-- =========================================================================
-- 3. Domination faction spawns in medium/hard anomalies
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2006,13518,789,0,0,-3500),      -- Domination Thug (Hidden Rally Point)
(2007,13523,790,0,0,-3500),      -- Domination Crusher (Forsaken Rally Point)
(2008,13529,790,0,0,-4000),      -- Domination Breaker (Port)
(2009,13535,848,0,0,-4500),      -- Domination Commander (Hub)
(2010,13535,848,0,0,-4500), (2010,13537,848,-3000,0,3500),  -- Commander + General (Hidden Hub)
(2011,13540,848,0,0,-4500), (2011,13539,848,3000,0,3500);   -- Saint + War General (Haven)

-- =========================================================================
-- 4. DED 3/10 renamed to Angel Repurposed Outpost (2300) + Domination sentries
-- =========================================================================
UPDATE `dunDungeons` SET `dungeonName` = 'Angel Repurposed Outpost' WHERE `dungeonID` = 2300;
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2300,17572,383,3000,0,1500), (2300,13114,383,-3000,0,1500),
(2301,17573,383,3000,0,1500), (2301,17605,383,-3000,0,1500),
(2302,17574,383,3000,0,1500), (2302,17575,383,-3000,0,1500);

-- =========================================================================
-- 5. Angel Hideaway (2000) rebuilt as wave-tiered pocket (Gistii frigates ->
--    Gistior destroyers, sentries), Burrow (2003) heavier with Gistum cruisers
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2000,2003);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hideaway: Gistii screen + destroyers, light battery
(2000,16901,597,0,0,1500), (2000,16902,597,-1500,0,0), (2000,16903,597,1500,0,-750),
(2000,24229,596,0,0,-2500), (2000,24230,596,-2500,0,1500),
(2000,13114,383,3500,0,2000), (2000,17572,383,-3500,0,2000),
(2000,2902,319,0,0,5000),
-- Burrow: Gistii + Gistum cruisers, heavier battery
(2003,16901,597,0,0,1500), (2003,16907,597,-1500,0,0), (2003,16903,597,1500,0,-750),
(2003,24229,596,0,0,-2500), (2003,16906,595,-2500,0,1500), (2003,16909,595,2500,0,-1500),
(2003,17572,383,3500,0,2000), (2003,17573,383,-3500,0,2000), (2003,17605,383,0,0,-4000),
(2003,2902,319,0,0,5000);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2012;
DELETE FROM `dunRooms` WHERE `roomID` = 2012;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2012;
