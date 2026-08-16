-- Sansha Nation content: sentry batteries in existing anomalies, new
-- Lookout/Refuge anomalies, DED 8/10/9/10/10 complexes, and wave-tiered
-- Den/Rally pockets. All NPC/structure typeIDs are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Sansha anomalies (roomID == dungeonID)
-- =========================================================================
-- Sansha Rally Point (2062) — light
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2062,17157,383,4000,0,1500), (2062,17580,383,-4000,0,1500);
-- Sansha Hidden Rally Point (2063) — medium
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2063,17156,383,4000,0,1500), (2063,17156,383,-4000,0,-1500), (2063,17581,383,0,0,4500);
-- Sansha Forsaken Rally Point (2064) — medium-hard
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2064,17156,383,4000,0,1500), (2064,17156,383,-4000,0,-1500), (2064,17582,383,0,0,4500);
-- Sansha Forlorn Rally Point (2065) — hard (True Sansha tier)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2065,16746,383,4000,0,1500), (2065,16746,383,-4000,0,-1500), (2065,17583,383,0,0,4500), (2065,17607,383,0,0,-4500);
-- Sansha Port (2066)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2066,17157,383,4000,0,1500), (2066,17580,383,-4000,0,1500), (2066,17607,383,0,0,4000);
-- Sansha Hidden Hub (2067) / Forsaken Hub (2068)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2067,17156,383,4500,0,1500), (2067,17156,383,-4500,0,-1500), (2067,17582,383,0,0,4500), (2067,17581,383,0,0,-4500),
(2068,17156,383,4500,0,1500), (2068,17156,383,-4500,0,-1500), (2068,17582,383,0,0,4500), (2068,17583,383,0,0,-4500);
-- Sansha Forlorn Hub (2069)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2069,16746,383,4500,0,1500), (2069,16746,383,-4500,0,-1500), (2069,16746,383,0,0,5000),
(2069,17582,383,4500,0,-2000), (2069,17583,383,-4500,0,-2000), (2069,17607,383,0,0,-4500);
-- Sansha Hub (2070)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2070,17156,383,4500,0,1500), (2070,17156,383,-4500,0,-1500), (2070,17156,383,0,0,5000),
(2070,17582,383,4500,0,-2000), (2070,17583,383,-4500,0,-2000), (2070,17607,383,0,0,-4500), (2070,28147,383,-2000,0,4000);
-- Sansha Haven (2071)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2071,16746,383,5000,0,1500), (2071,16746,383,-5000,0,-1500), (2071,16746,383,0,0,5500),
(2071,17582,383,5000,0,-2000), (2071,17582,383,-5000,0,-2000), (2071,17583,383,0,0,-5000), (2071,28152,383,-2000,0,4500);
-- Sansha Sanctum (2072) — heavily defended
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2072,16746,383,5500,0,1500), (2072,16746,383,-5500,0,-1500), (2072,16746,383,0,0,6000), (2072,16746,383,0,0,-6000),
(2072,17583,383,5500,0,-2000), (2072,17583,383,-5500,0,-2000), (2072,17607,383,3000,0,4000), (2072,17607,383,-3000,0,4000),
(2072,28152,383,2000,0,5500), (2072,28152,383,-2000,0,-5500);

-- =========================================================================
-- 2. Sansha Lookout (2094) — two pockets: light gate screen, then a
--    "Control Center" command room with a True Sansha bonus wave
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2094,2095);
DELETE FROM `dunRooms` WHERE `roomID` IN (2094,2095);
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2094;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2094, 'Sansha Lookout', 0, 500019, 7, 'sansha-lookout');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2094,'Lookout Entry',2094), (2095,'Lookout Command',2094);

-- Entry: light screen of frigates/destroyers + a sentry (guarding the gate)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2094,10025,567,0,0,1500), (2094,10025,567,-1500,0,0), (2094,10025,567,1500,0,-750),
(2094,23378,581,0,0,-2500), (2094,23379,581,-2500,0,1500),
(2094,17157,383,4000,0,1500), (2094,17580,383,-4000,0,1500),
(2094,2902,319,0,0,5000);
-- Command: Sansha Control Center + cruiser/BC escort + True Sansha bonus
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2095,26249,494,0,0,0),                      -- Sansha Control Center (boss)
(2095,23383,582,1500,0,-750), (2095,23383,582,-1500,0,0),
(2095,11913,565,0,0,3000), (2095,23391,809,0,0,-3000),   -- True Sansha's Misshape bonus
(2095,17156,383,4000,0,1500), (2095,17581,383,-4000,0,1500);

-- =========================================================================
-- 3. Sansha Refuge (2096) — ground bunkers/watchtowers, frigate->destroyer
--    waves with an elite True Sansha presence
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` = 2096;
DELETE FROM `dunRooms` WHERE `roomID` = 2096;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2096;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2096, 'Sansha Refuge', 0, 500019, 7, 'sansha-refuge');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2096,'Refuge Pocket',2096);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2096,10025,567,0,0,1500), (2096,10025,567,-1500,0,0), (2096,10026,567,1500,0,-750),
(2096,10030,566,0,0,-2500), (2096,10030,566,-2500,0,1500),
(2096,23378,581,2500,0,-1500), (2096,23379,581,0,0,3500),
(2096,23391,809,0,0,-4000),                       -- True Sansha's Misshape (elite)
(2096,17580,383,3500,0,2000), (2096,17581,383,-3500,0,2000);

-- =========================================================================
-- 4. DED 8/10 — Sansha Prison Camp (2730), 4 rooms, "Central Bastion" boss
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2730 AND 2733;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2730 AND 2733;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2730;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2730, 'Sansha Prison Camp', 0, 500019, 10, 'sansha-ded-8of10');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2730,'Prison Gate',2730), (2731,'Prison Block',2730), (2732,'Prison Yard',2730), (2733,'Central Bastion',2730);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1: light screen
(2730,10025,567,0,0,1500), (2730,10025,567,-1500,0,0), (2730,10030,566,1500,0,-750),
(2730,17157,383,3500,0,1500), (2730,17580,383,-3500,0,1500),
(2730,2902,319,0,0,5000),
-- Room 2: escalating
(2731,10030,566,0,0,1500), (2731,10030,566,-1500,0,0), (2731,23383,582,1500,0,-750),
(2731,17580,383,3500,0,1500), (2731,17607,383,-3500,0,1500),
(2731,2902,319,0,0,5000),
-- Room 3: heavy
(2732,23383,582,0,0,1500), (2732,23383,582,-1500,0,0), (2732,11913,565,1500,0,-750),
(2732,17156,383,3500,0,1500), (2732,17582,383,-3500,0,1500), (2732,28147,383,0,0,4500),
(2732,2902,319,0,0,5000),
-- Room 4: Central Bastion boss (3805) + True Sansha escort
(2733,3805,495,0,0,0),                  -- Sansha's Nation Central Bastion
(2733,13606,851,1500,0,-750), (2733,13606,851,-1500,0,0),
(2733,19737,621,0,0,3000),              -- Sansha's Tyrant Elite
(2733,16746,383,4000,0,1500), (2733,17583,383,-4000,0,1500), (2733,17607,383,0,0,-4000);

-- =========================================================================
-- 5. DED 9/10 — True Sansha Fleet Staging Point (2830), 3 rooms, staging
--    that escalates through True Sansha fleet to the True Battle Tower
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2830 AND 2832;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2830 AND 2832;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2830;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2830, 'True Sansha Fleet Staging Point', 0, 500019, 10, 'sansha-ded-9of10');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2830,'Staging Entry',2830), (2831,'Fleet Command',2830), (2832,'True Battle Tower',2830);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2830,10030,566,0,0,1500), (2830,23383,582,-1500,0,0), (2830,23383,582,1500,0,-750),
(2830,17581,383,3500,0,1500), (2830,17156,383,-3500,0,1500),
(2830,2902,319,0,0,5000),
-- Room 2: True Sansha fleet
(2831,23394,807,0,0,1500), (2831,23394,807,-1500,0,0), (2831,13606,851,1500,0,-750),
(2831,16746,383,3500,0,1500), (2831,17583,383,-3500,0,1500), (2831,28147,383,0,0,4500),
(2831,2902,319,0,0,5000),
-- Room 3: Sansha Fleet Outpost as the True Battle Tower boss
(2832,29023,495,0,0,0),                 -- Sansha Fleet Outpost
(2832,23403,851,1500,0,-750), (2832,23403,851,-1500,0,0),
(2832,16746,383,4000,0,1500), (2832,17583,383,-4000,0,1500), (2832,28152,383,0,0,-4000);

-- =========================================================================
-- 6. DED 10/10 — Centus Assembly T.P. Co. (2930), 5 rooms, "Station Ultima" boss
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2930 AND 2934;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2930 AND 2934;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2930;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2930, 'Centus Assembly T.P. Co.', 0, 500019, 10, 'sansha-ded-10of10');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2930,'Assembly Entry',2930), (2931,'Production Line',2930), (2932,'Assembly Hall',2930),
(2933,'Control Deck',2930), (2934,'Station Ultima',2930);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2930,10030,566,0,0,1500), (2930,10025,567,-1500,0,0), (2930,17580,383,3500,0,1500),
(2930,2902,319,0,0,5000),
-- Room 2
(2931,10030,566,0,0,1500), (2931,23383,582,-1500,0,0), (2931,17582,383,3500,0,1500), (2931,17607,383,-3500,0,1500),
(2931,2902,319,0,0,5000),
-- Room 3
(2932,23383,582,0,0,1500), (2932,11913,565,-1500,0,0), (2932,11913,565,1500,0,-750),
(2932,17156,383,3500,0,1500), (2932,17582,383,-3500,0,1500),
(2932,2902,319,0,0,5000),
-- Room 4
(2933,23394,807,0,0,1500), (2933,13606,851,-1500,0,0), (2933,17583,383,3500,0,1500), (2933,17607,383,-3500,0,1500),
(2933,2902,319,0,0,5000),
-- Room 5: Station Ultima boss
(2934,19961,495,0,0,0),                 -- Station Ultima
(2934,23403,851,1500,0,-750), (2934,23403,851,-1500,0,0),
(2934,19737,621,0,0,3000),
(2934,16746,383,4000,0,1500), (2934,17583,383,-4000,0,1500), (2934,28152,383,0,0,-4000);

-- =========================================================================
-- 7. Sansha Forsaken Den (2060) / Forlorn Den (2061) — wave-tiered pockets:
--    frigates in close, destroyers mid, cruisers/BC on the flanks, a battleship
--    deep — with sentry batteries (replaces the stray Blood Raider legacy rats)
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2060,2061);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Den (light): frigate screen + destroyers + 1 cruiser, light battery
(2060,10025,567,0,0,1500), (2060,10025,567,-1500,0,0), (2060,10026,567,1500,0,-750),
(2060,23378,581,0,0,-2500), (2060,23379,581,-2500,0,1500),
(2060,10030,566,2500,0,-1500),
(2060,17580,383,3500,0,2000), (2060,17581,383,-3500,0,2000),
(2060,2902,319,0,0,5000),
-- Forlorn Den (harder): + more destroyers/cruisers, battleship deep, True Sansha elite
(2061,10025,567,0,0,1500), (2061,10026,567,-1500,0,0), (2061,10027,567,1500,0,-750),
(2061,23378,581,0,0,-2500), (2061,23379,581,-2500,0,1500), (2061,23405,581,2500,0,-1500),
(2061,10030,566,0,0,-3500), (2061,23383,582,3000,0,2000),
(2061,11913,565,0,0,5000), (2061,23391,809,-3000,0,3000),   -- True Sansha's Misshape
(2061,17580,383,3500,0,2000), (2061,17582,383,-3500,0,2000), (2061,17607,383,0,0,-4500),
(2061,2902,319,0,0,5000);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2094 AND 2096 OR `roomID` BETWEEN 2730 AND 2733 OR `roomID` BETWEEN 2830 AND 2832 OR `roomID` BETWEEN 2930 AND 2934;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2094 AND 2096 OR `roomID` BETWEEN 2730 AND 2733 OR `roomID` BETWEEN 2830 AND 2832 OR `roomID` BETWEEN 2930 AND 2934;
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2094,2096,2730,2830,2930);
