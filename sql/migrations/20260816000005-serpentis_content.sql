-- Serpentis content: sentry batteries in existing anomalies, new Rally/Port/
-- Hub/Haven/Sanctum, DED Pharmalogical Plant + Research Complex, Shadow
-- faction spawns, wave-tiered pockets. All types are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Serpentis anomalies (roomID == dungeonID)
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Forsaken Hideaway (2081) / Forlorn Hideaway (2082)
(2081,17568,383,3500,0,2000), (2081,17569,383,-3500,0,-2000), (2081,17570,383,0,0,4000),
(2082,17568,383,3500,0,2000), (2082,17569,383,-3500,0,-2000), (2082,17163,383,0,0,4000),
-- Burrow (2083) / Refuge (2084)
(2083,17568,383,3500,0,2000), (2083,17569,383,-3500,0,-2000), (2083,17163,383,0,0,-4000),
(2084,17568,383,3500,0,2000), (2084,17570,383,-3500,0,-2000), (2084,28143,383,0,0,4000),
-- Forsaken Rally Point (2085) / Forsaken Hub (2086)
(2085,17570,383,3500,0,2000), (2085,17571,383,-3500,0,-2000), (2085,17163,383,0,0,4000),
(2086,17570,383,4500,0,1500), (2086,17571,383,-4500,0,-1500), (2086,17163,383,0,0,4500),
(2086,28143,383,-2000,0,4000), (2086,28148,383,2000,0,4000);

-- =========================================================================
-- 2. Serpentis Rally Point (2087) / Port (2088) / Hub (2089)
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2087,2088,2089);
DELETE FROM `dunRooms` WHERE `roomID` IN (2087,2088,2089);
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2087,2088,2089);

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2087, 'Serpentis Rally Point', 0, 500013, 7, 'serpentis-rallypoint'),
(2088, 'Serpentis Port', 0, 500013, 7, 'serpentis-port'),
(2089, 'Serpentis Hub', 0, 500013, 7, 'serpentis-hub');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2087,'Rally Pocket',2087), (2088,'Port Pocket',2088), (2089,'Hub Pocket',2089);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Rally Point: Coreli/Corelior + Corelum cruisers
(2087,17118,633,0,0,1500), (2087,17116,633,-1500,0,0), (2087,17120,633,1500,0,-750),
(2087,23973,632,0,0,-2500), (2087,23994,632,-2500,0,1500),
(2087,17039,631,2500,0,-1500), (2087,17110,631,0,0,3500),
(2087,17568,383,3500,0,2000), (2087,17570,383,-3500,0,2000), (2087,17163,383,0,0,-4500),
-- Port: Corelior + Corelatis BCs
(2088,23973,632,0,0,1500), (2088,23995,632,-1500,0,0), (2088,23996,632,1500,0,-750),
(2088,17039,631,0,0,-2500), (2088,24063,631,-2500,0,1500),
(2088,24003,629,2500,0,-1500), (2088,24026,629,0,0,3500),
(2088,17570,383,3500,0,2000), (2088,17571,383,-3500,0,2000), (2088,28143,383,0,0,-4500),
-- Hub: Corelatis + Core battleships
(2089,24003,629,0,0,1500), (2089,24026,629,-1500,0,0), (2089,24028,629,1500,0,-750),
(2089,17034,630,0,0,-2500), (2089,17037,630,-2500,0,1500), (2089,24167,630,2500,0,-1500),
(2089,17570,383,4500,0,2000), (2089,17571,383,-4500,0,2000), (2089,17163,383,0,0,-4500), (2089,28148,383,-2000,0,4000);

-- =========================================================================
-- 3. Serpentis Haven (2050) / Sanctum (2051) — heavy narcosyndicate strongholds
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2050,2051);
DELETE FROM `dunRooms` WHERE `roomID` IN (2050,2051);
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2050,2051);

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2050, 'Serpentis Haven', 0, 500013, 7, 'serpentis-haven'),
(2051, 'Serpentis Sanctum', 0, 500013, 7, 'serpentis-sanctum');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2050,'Haven Pocket',2050), (2051,'Sanctum Pocket',2051);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Haven: Corelatis + Core admirals + Shadow
(2050,24003,629,0,0,1500), (2050,24026,629,-1500,0,0), (2050,24028,629,1500,0,-750),
(2050,24167,630,0,0,-2500), (2050,24168,630,-2500,0,1500), (2050,24170,630,2500,0,-1500),
(2050,23469,852,0,0,-4000),                       -- Shadow Serpentis Admiral
(2050,23471,852,3000,0,3000),                     -- Shadow Serpentis Grand Admiral
(2050,17570,383,4500,0,2000), (2050,17571,383,-4500,0,2000), (2050,17163,383,0,0,-4500), (2050,28148,383,-2000,0,4000),
-- Sanctum: Core fleet + Shadow elite
(2051,24167,630,0,0,1500), (2051,24169,630,-1500,0,0), (2051,24170,630,1500,0,-750),
(2051,24165,630,0,0,-2500), (2051,24168,630,-2500,0,1500),
(2051,23471,852,0,0,-4000),                       -- Shadow Serpentis Grand Admiral
(2051,23472,852,3000,0,3000),                     -- Shadow Serpentis Lord Admiral
(2051,23469,852,-3000,0,3000),                    -- Shadow Serpentis Admiral
(2051,17571,383,4500,0,2000), (2051,28148,383,-4500,0,2000), (2051,17163,383,0,0,-4500), (2051,28153,383,-2000,0,4000);

-- =========================================================================
-- 4. Shadow Serpentis faction spawns in medium/hard anomalies
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2085,23457,813,0,0,-3500),      -- Shadow Serpentis Trooper (Forsaken Rally)
(2087,23463,811,0,0,-4000),      -- Shadow Serpentis Wing Leader (Rally Point)
(2088,23466,811,0,0,-4000),      -- Shadow Serpentis Captain (Port)
(2089,23469,852,0,0,-4500),      -- Shadow Serpentis Admiral (Hub)
(2086,23471,852,0,0,-4500), (2086,23470,852,-3000,0,3500);  -- Grand + High Admiral (Forsaken Hub)

-- =========================================================================
-- 5. DED Serpentis Pharmalogical Plant (2720) — narcotics production
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2720 AND 2723;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2720 AND 2723;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2720;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2720, 'Serpentis Pharmalogical Plant', 0, 500013, 10, 'serpentis-ded-plant');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2720,'Plant Gate',2720), (2721,'Production Line',2720), (2722,'Laboratory',2720), (2723,'Plant Core',2720);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2720,17116,633,0,0,1500), (2720,17118,633,-1500,0,0), (2720,23973,632,1500,0,-750),
(2720,13115,383,3000,0,1500), (2720,17568,383,-3000,0,1500),
(2720,2902,319,0,0,5000),
-- Room 2: narcotics storage platforms (Crash/Exile/Mindflood)
(2721,23973,632,0,0,1500), (2721,23995,632,-1500,0,0), (2721,17039,631,1500,0,-750),
(2721,26856,383,3000,0,1500), (2721,26857,383,-3000,0,1500), (2721,26858,383,0,0,4500),
(2721,2902,319,0,0,5000),
-- Room 3
(2722,17039,631,0,0,1500), (2722,24003,629,-1500,0,0), (2722,24026,629,1500,0,-750),
(2722,17570,383,3000,0,1500), (2722,17163,383,-3000,0,1500), (2722,28148,383,0,0,4000),
(2722,2902,319,0,0,5000),
-- Room 4: Serpentis Control Center + Core admirals + Shadow
(2723,26250,494,0,0,0),                  -- Serpentis Control Center
(2723,24167,630,1500,0,-750), (2723,24170,630,-1500,0,0), (2723,24165,630,0,0,3000),
(2723,23471,852,0,0,-3000),              -- Shadow Serpentis Grand Admiral
(2723,17571,383,4000,0,1500), (2723,28148,383,-4000,0,1500), (2723,17163,383,0,0,-4000);

-- =========================================================================
-- 6. DED Serpentis Research Complex (2920) — black-implants R&D
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2920 AND 2924;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2920 AND 2924;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2920;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2920, 'Serpentis Research Complex', 0, 500013, 10, 'serpentis-ded-research');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2920,'Research Gate',2920), (2921,'Laboratory Wing',2920), (2922,'Genetics Lab',2920),
(2923,'Implant Vault',2920), (2924,'Operational HQ',2920);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2920,17118,633,0,0,1500), (2920,17119,633,-1500,0,0), (2920,23973,632,1500,0,-750),
(2920,13115,383,3000,0,1500), (2920,17568,383,-3000,0,1500),
(2920,2902,319,0,0,5000),
-- Room 2
(2921,23973,632,0,0,1500), (2921,23995,632,-1500,0,0), (2921,17039,631,1500,0,-750),
(2921,17569,383,3000,0,1500), (2921,17163,383,-3000,0,1500),
(2921,2902,319,0,0,5000),
-- Room 3
(2922,17039,631,0,0,1500), (2922,24003,629,-1500,0,0), (2922,24026,629,1500,0,-750),
(2922,17570,383,3000,0,1500), (2922,28148,383,-3000,0,1500),
(2922,2902,319,0,0,5000),
-- Room 4
(2923,24003,629,0,0,1500), (2923,24167,630,-1500,0,0), (2923,24168,630,1500,0,-750),
(2923,17571,383,3000,0,1500), (2923,28148,383,-3000,0,1500), (2923,17163,383,0,0,4000),
(2923,2902,319,0,0,5000),
-- Room 5: Core Serpentis Operational HQ + Shadow fleet
(2924,24579,494,0,0,0),                  -- Core Serpentis Operational Headquarters
(2924,24169,630,1500,0,-750), (2924,24170,630,-1500,0,0), (2924,24165,630,0,0,3000),
(2924,23472,852,0,0,-3000),              -- Shadow Serpentis Lord Admiral
(2924,23471,852,3000,0,2000),            -- Shadow Serpentis Grand Admiral
(2924,17571,383,4000,0,1500), (2924,28153,383,-4000,0,1500), (2924,17163,383,0,0,-4000);

-- =========================================================================
-- 7. Serpentis Hideaway (2080) rebuilt as wave-tiered pocket, clearing the
--    stray Renegade Serpentis Assassin legacy rats
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2080;
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2080,17116,633,0,0,1500), (2080,17118,633,-1500,0,0), (2080,17121,633,1500,0,-750),
(2080,23973,632,0,0,-2500), (2080,23994,632,-2500,0,1500),
(2080,13115,383,3500,0,2000), (2080,17568,383,-3500,0,2000),
(2080,2902,319,0,0,5000);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2087,2088,2089,2050,2051) OR `roomID` BETWEEN 2720 AND 2723 OR `roomID` BETWEEN 2920 AND 2924;
DELETE FROM `dunRooms` WHERE `roomID` IN (2087,2088,2089,2050,2051) OR `roomID` BETWEEN 2720 AND 2723 OR `roomID` BETWEEN 2920 AND 2924;
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2087,2088,2089,2050,2051,2720,2920);
