-- DED combat anomaly dungeons by faction
-- ArchetypeID=7 (Anomaly) for basic combat sites
-- +migrate Up

-- ====== Angel Cartel (500011) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2000, 'Angel Hideaway', 3, 500011, 7, 'angel-anom-hideaway'),
(2001, 'Angel Hidden Hideaway', 3, 500011, 7, 'angel-anom-hidden'),
(2002, 'Angel Forlorn Hideaway', 3, 500011, 7, 'angel-anom-forlorn'),
(2003, 'Angel Burrow', 3, 500011, 7, 'angel-anom-burrow'),
(2004, 'Angel Refuge', 3, 500011, 7, 'angel-anom-refuge'),
(2005, 'Angel Den', 3, 500011, 7, 'angel-anom-den'),
(2006, 'Angel Hidden Rally Point', 3, 500011, 7, 'angel-anom-hrp'),
(2007, 'Angel Forsaken Rally Point', 3, 500011, 7, 'angel-anom-frp'),
(2008, 'Angel Port', 3, 500011, 7, 'angel-anom-port'),
(2009, 'Angel Hub', 3, 500011, 7, 'angel-anom-hub'),
(2010, 'Angel Hidden Hub', 3, 500011, 7, 'angel-anom-hhub'),
(2011, 'Angel Haven', 3, 500011, 7, 'angel-anom-haven');

-- ====== Blood Raiders (500012) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2020, 'Blood Hideaway', 3, 500012, 7, 'blood-anom-hideaway'),
(2021, 'Blood Forsaken Hideaway', 3, 500012, 7, 'blood-anom-forsaken'),
(2022, 'Blood Refuge', 3, 500012, 7, 'blood-anom-refuge'),
(2023, 'Blood Den', 3, 500012, 7, 'blood-anom-den'),
(2024, 'Blood Hidden Den', 3, 500012, 7, 'blood-anom-hden'),
(2025, 'Blood Yard', 3, 500012, 7, 'blood-anom-yard'),
(2026, 'Blood Rally Point', 3, 500012, 7, 'blood-anom-rpoint'),
(2027, 'Blood Port', 3, 500012, 7, 'blood-anom-port'),
(2028, 'Blood Hub', 3, 500012, 7, 'blood-anom-hub'),
(2029, 'Blood Forsaken Hub', 3, 500012, 7, 'blood-anom-fhub');

-- ====== Guristas (500010) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2040, 'Guristas Hideaway', 3, 500010, 7, 'guristas-anom-hideaway'),
(2041, 'Guristas Hidden Hideaway', 3, 500010, 7, 'guristas-anom-hidden'),
(2042, 'Guristas Forsaken Hideaway', 3, 500010, 7, 'guristas-anom-forsaken'),
(2043, 'Guristas Forlorn Hideaway', 3, 500010, 7, 'guristas-anom-forlorn'),
(2044, 'Guristas Burrow', 3, 500010, 7, 'guristas-anom-burrow'),
(2045, 'Guristas Refuge', 3, 500010, 7, 'guristas-anom-refuge'),
(2046, 'Guristas Den', 3, 500010, 7, 'guristas-anom-den'),
(2047, 'Guristas Hub', 3, 500010, 7, 'guristas-anom-hub'),
(2048, 'Guristas Forlorn Hub', 3, 500010, 7, 'guristas-anom-fhub');

-- ====== Sansha's Nation (500019) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2060, 'Sansha Forsaken Den', 3, 500019, 7, 'sansha-anom-fden'),
(2061, 'Sansha Forlorn Den', 3, 500019, 7, 'sansha-anom-lden'),
(2062, 'Sansha Rally Point', 3, 500019, 7, 'sansha-anom-rpoint'),
(2063, 'Sansha Hidden Rally Point', 3, 500019, 7, 'sansha-anom-hrp'),
(2064, 'Sansha Forsaken Rally Point', 3, 500019, 7, 'sansha-anom-frp'),
(2065, 'Sansha Forlorn Rally Point', 3, 500019, 7, 'sansha-anom-lrp'),
(2066, 'Sansha Port', 3, 500019, 7, 'sansha-anom-port'),
(2067, 'Sansha Hidden Hub', 3, 500019, 7, 'sansha-anom-hhub'),
(2068, 'Sansha Forsaken Hub', 3, 500019, 7, 'sansha-anom-fhub'),
(2069, 'Sansha Forlorn Hub', 3, 500019, 7, 'sansha-anom-lhub'),
(2070, 'Sansha Hub', 3, 500019, 7, 'sansha-anom-hub'),
(2071, 'Sansha Haven', 3, 500019, 7, 'sansha-anom-haven'),
(2072, 'Sansha Sanctum', 3, 500019, 7, 'sansha-anom-sanctum');

-- ====== Serpentis (500013) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2080, 'Serpentis Hideaway', 3, 500013, 7, 'serp-anom-hideaway'),
(2081, 'Serpentis Forsaken Hideaway', 3, 500013, 7, 'serp-anom-forsaken'),
(2082, 'Serpentis Forlorn Hideaway', 3, 500013, 7, 'serp-anom-forlorn'),
(2083, 'Serpentis Burrow', 3, 500013, 7, 'serp-anom-burrow'),
(2084, 'Serpentis Refuge', 3, 500013, 7, 'serp-anom-refuge'),
(2085, 'Serpentis Forsaken Rally Point', 3, 500013, 7, 'serp-anom-frp'),
(2086, 'Serpentis Forsaken Hub', 3, 500013, 7, 'serp-anom-fhub');

-- ====== Rogue Drones (500022) ======
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2090, 'Drone Cluster', 3, 500022, 7, 'drone-anom-cluster'),
(2091, 'Drone Collection', 3, 500022, 7, 'drone-anom-collection'),
(2092, 'Drone Assembly', 3, 500022, 7, 'drone-anom-assembly'),
(2093, 'Drone Horde', 3, 500022, 7, 'drone-anom-horde');

-- Rooms for each dungeon
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2000, 'Hideaway Pocket', 2000), (2001, 'Hideaway Pocket', 2001),
(2002, 'Hideaway Pocket', 2002), (2003, 'Burrow Pocket', 2003),
(2004, 'Refuge Pocket', 2004), (2005, 'Den Pocket', 2005),
(2006, 'Rally Point Pocket', 2006), (2007, 'Rally Point Pocket', 2007),
(2008, 'Port Pocket', 2008), (2009, 'Hub Pocket', 2009),
(2010, 'Hub Pocket', 2010), (2011, 'Haven Pocket', 2011),
(2020, 'Hideaway Pocket', 2020), (2021, 'Hideaway Pocket', 2021),
(2022, 'Refuge Pocket', 2022), (2023, 'Den Pocket', 2023),
(2024, 'Den Pocket', 2024), (2025, 'Yard Pocket', 2025),
(2026, 'Rally Point Pocket', 2026), (2027, 'Port Pocket', 2027),
(2028, 'Hub Pocket', 2028), (2029, 'Hub Pocket', 2029),
(2040, 'Hideaway Pocket', 2040), (2041, 'Hideaway Pocket', 2041),
(2042, 'Hideaway Pocket', 2042), (2043, 'Hideaway Pocket', 2043),
(2044, 'Burrow Pocket', 2044), (2045, 'Refuge Pocket', 2045),
(2046, 'Den Pocket', 2046), (2047, 'Hub Pocket', 2047),
(2048, 'Hub Pocket', 2048),
(2060, 'Den Pocket', 2060), (2061, 'Den Pocket', 2061),
(2062, 'Rally Point Pocket', 2062), (2063, 'Rally Point Pocket', 2063),
(2064, 'Rally Point Pocket', 2064), (2065, 'Rally Point Pocket', 2065),
(2066, 'Port Pocket', 2066), (2067, 'Hub Pocket', 2067),
(2068, 'Hub Pocket', 2068), (2069, 'Hub Pocket', 2069),
(2070, 'Hub Pocket', 2070), (2071, 'Haven Pocket', 2071),
(2072, 'Sanctum Pocket', 2072),
(2080, 'Hideaway Pocket', 2080), (2081, 'Hideaway Pocket', 2081),
(2082, 'Hideaway Pocket', 2082), (2083, 'Burrow Pocket', 2083),
(2084, 'Refuge Pocket', 2084), (2085, 'Rally Point Pocket', 2085),
(2086, 'Hub Pocket', 2086),
(2090, 'Drone Pocket', 2090), (2091, 'Drone Pocket', 2091),
(2092, 'Drone Pocket', 2092), (2093, 'Drone Pocket', 2093);

-- +migrate Down
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2000 AND 2093;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2000 AND 2093;
