-- Blood Raiders content: sentry batteries in existing anomalies, new
-- Haven/Sanctum, DED Temple Complex, Dark Blood faction spawns, wave-tiered
-- pockets. All types are real SDE types.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Blood anomalies (roomID == dungeonID)
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hideaway (2020) / Forsaken Hideaway (2021)
(2021,17592,383,3500,0,2000), (2021,17592,383,-3500,0,-2000), (2021,17593,383,0,0,4000),
-- Refuge (2022) / Den (2023) / Hidden Den (2024)
(2022,17592,383,3500,0,2000), (2022,17592,383,-3500,0,-2000), (2022,17593,383,0,0,4000), (2022,17610,383,0,0,-4000),
(2024,17592,383,3500,0,2000), (2024,17594,383,-3500,0,2000), (2024,17610,383,0,0,-4000),
-- Yard (2025) / Rally Point (2026)
(2025,17593,383,3500,0,2000), (2025,17594,383,-3500,0,-2000), (2025,27281,383,0,0,4000),
(2026,17594,383,3500,0,2000), (2026,17595,383,-3500,0,-2000), (2026,17610,383,0,0,4000),
-- Port (2027)
(2027,17594,383,3500,0,2000), (2027,17595,383,-3500,0,-2000), (2027,17610,383,0,0,4000), (2027,28140,383,-2000,0,4500),
-- Hub (2028) / Forsaken Hub (2029)
(2028,17144,383,4500,0,1500), (2028,17144,383,-4500,0,-1500), (2028,17594,383,0,0,4500),
(2028,17595,383,0,0,-4500), (2028,17610,383,-2000,0,4000), (2028,28145,383,2000,0,4000),
(2029,16741,383,4500,0,1500), (2029,16741,383,-4500,0,-1500), (2029,16741,383,0,0,5000),
(2029,17595,383,4500,0,-2000), (2029,17595,383,-4500,0,-2000), (2029,17610,383,0,0,-4500),
(2029,28149,383,2000,0,4500), (2029,28149,383,-2000,0,-4500);

-- =========================================================================
-- 2. Blood Haven (2030) / Blood Sanctum (2031) — heavy cult strongholds
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2030,2031);
DELETE FROM `dunRooms` WHERE `roomID` IN (2030,2031);
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2030,2031);

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2030, 'Blood Haven', 0, 500012, 7, 'blood-haven'),
(2031, 'Blood Sanctum', 0, 500012, 7, 'blood-sanctum');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2030,'Haven Pocket',2030), (2031,'Sanctum Pocket',2031);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Haven: Corpatis BCs + Corpus battleships + Dark Blood
(2030,24000,602,0,0,1500), (2030,24009,602,-1500,0,0), (2030,24012,602,1500,0,-750),
(2030,16934,603,0,0,-2500), (2030,16938,603,-2500,0,1500), (2030,24136,603,2500,0,-1500),
(2030,23301,849,0,0,-4000),                       -- Dark Blood Patriarch
(2030,23302,849,3000,0,3000),                     -- Dark Blood Pope
(2030,17144,383,4500,0,2000), (2030,17595,383,-4500,0,2000), (2030,17610,383,0,0,-4500), (2030,28145,383,-2000,0,4000),
-- Sanctum: Corpus fleet + Dark Blood elite
(2031,16938,603,0,0,1500), (2031,16941,603,-1500,0,0), (2031,24139,603,1500,0,-750),
(2031,24140,603,0,0,-2500), (2031,26754,603,-2500,0,1500),
(2031,13559,849,0,0,-4000),                       -- Dark Blood Oracle
(2031,13562,849,3000,0,3000),                     -- Dark Blood Apostle
(2031,13560,849,-3000,0,3000),                    -- Dark Blood Archbishop
(2031,16741,383,4500,0,2000), (2031,17595,383,-4500,0,2000), (2031,17610,383,0,0,-4500), (2031,28149,383,-2000,0,4000);

-- =========================================================================
-- 3. Dark Blood faction spawns in medium/hard anomalies
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2025,23287,796,0,0,-3500),      -- Dark Blood Visionary (Yard)
(2026,23293,795,0,0,-3500),      -- Dark Blood Bishop (Rally Point)
(2027,23298,795,0,0,-4000),      -- Dark Blood Exorcist (Port)
(2028,23300,849,0,0,-4500),      -- Dark Blood Cardinal (Hub)
(2029,23301,849,0,0,-4500), (2029,23299,849,-3000,0,3500);  -- Patriarch + Monsignor (Forsaken Hub)

-- =========================================================================
-- 4. DED Temple Complex (2710) — holy cult site, Corpus + Dark Blood guards
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2710 AND 2713;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2710 AND 2713;
DELETE FROM `dunDungeons` WHERE `dungeonID` = 2710;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2710, 'Blood Raider Temple Complex', 0, 500012, 10, 'blood-ded-temple');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2710,'Temple Gate',2710), (2711,'Sanctuary',2710), (2712,'Inner Sanctum',2710), (2713,'Temple Core',2710);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Room 1
(2710,23970,605,0,0,1500), (2710,23979,605,-1500,0,0), (2710,23980,605,1500,0,-750),
(2710,13116,383,3000,0,1500), (2710,17592,383,-3000,0,1500),
(2710,2902,319,0,0,5000),
-- Room 2: neutralizer + stasis defenses per lore
(2711,16927,604,0,0,1500), (2711,16928,604,-1500,0,0), (2711,16929,604,1500,0,-750),
(2711,24000,602,0,0,-2500), (2711,24009,602,-2500,0,1500),
(2711,17593,383,3000,0,1500), (2711,17610,383,-3000,0,1500), (2711,28145,383,0,0,4000),
(2711,2902,319,0,0,5000),
-- Room 3
(2712,24000,602,0,0,1500), (2712,24012,602,-1500,0,0), (2712,16934,603,1500,0,-750),
(2712,16938,603,0,0,-2500),
(2712,17594,383,3000,0,1500), (2712,17610,383,-3000,0,1500), (2712,28145,383,0,0,4000),
(2712,2902,319,0,0,5000),
-- Room 4: Blood Raider Control Center + Corpus clergy + Dark Blood
(2713,26247,494,0,0,0),                  -- Blood Raider Control Center
(2713,24139,603,1500,0,-750), (2713,24140,603,-1500,0,0), (2713,16941,603,0,0,3000),
(2713,13559,849,0,0,-3000),              -- Dark Blood Oracle
(2713,13562,849,3000,0,2000),            -- Dark Blood Apostle
(2713,16741,383,4000,0,1500), (2713,17595,383,-4000,0,1500), (2713,28149,383,0,0,-4000), (2713,17610,383,-2000,0,4000);

-- =========================================================================
-- 5. Blood Hideaway (2020) rebuilt as wave-tiered pocket, Den (2023) cleared
--    of stray Renegade Guristas legacy rats and given priestly escort
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2020,2023);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Hideaway: Corpior screen + Corpum, light battery
(2020,23970,605,0,0,1500), (2020,23979,605,-1500,0,0), (2020,23980,605,1500,0,-750),
(2020,16944,604,0,0,-2500), (2020,16948,604,-2500,0,1500),
(2020,13116,383,3500,0,2000), (2020,17592,383,-3500,0,2000),
(2020,2902,319,0,0,5000),
-- Den: Corpior + Corpum + Corpatis, heavier batteries
(2023,23970,605,0,0,1500), (2023,23981,605,-1500,0,0), (2023,23982,605,1500,0,-750),
(2023,16927,604,0,0,-2500), (2023,16930,604,-2500,0,1500), (2023,16929,604,2500,0,-1500),
(2023,24000,602,0,0,3500), (2023,24013,602,-3000,0,2500),
(2023,17592,383,3500,0,2000), (2023,17594,383,-3500,0,2000), (2023,17610,383,0,0,-4000),
(2023,2902,319,0,0,5000);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` IN (2030,2031) OR `roomID` BETWEEN 2710 AND 2713;
DELETE FROM `dunRooms` WHERE `roomID` IN (2030,2031) OR `roomID` BETWEEN 2710 AND 2713;
DELETE FROM `dunDungeons` WHERE `dungeonID` IN (2030,2031,2710);
