-- Additional career agent mission chains: Industry, Business, Exploration
-- Tutorial IDs: 1011-1030 (Industry), 1031-1040 (Business), 1041-1045 (Exploration)
-- +migrate Up

-- ============================================================
-- Making Mountains of Molehills — Gallente Industry (10 missions)
-- ============================================================
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`, `dataID`) VALUES
(1011, 'Industry: Mission 1 - Mining 101', 1012, 3, 0),
(1012, 'Industry: Mission 2 - Tritanium Delivery', 1013, 3, 0),
(1013, 'Industry: Mission 3 - Afterburner Assembly', 1014, 3, 0),
(1014, 'Industry: Mission 4 - Bulk Mining', 1015, 3, 0),
(1015, 'Industry: Mission 5 - Electronic Parts', 1016, 3, 0),
(1016, 'Industry: Mission 6 - Cap Boosters', 1017, 3, 0),
(1017, 'Industry: Mission 7 - Courier Run', 1018, 3, 0),
(1018, 'Industry: Mission 8 - Shuttle Manufacturing', 1019, 3, 0),
(1019, 'Industry: Mission 9 - Production Assistant', 1020, 3, 0),
(1020, 'Industry: Mission 10 - Navitas Construction', 0, 3, 0);

INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(2011, 1011, 1, 'Mining 101',
 'Your agent wants you to mine 1000 units of Veldspar. Fit your Mining Laser, warp to the asteroid belt, lock onto a Veldspar asteroid, and activate your miner. Keep mining until you have 1000 units, then return to station.', '', ''),
(2012, 1012, 1, 'Tritanium Delivery',
 'Your agent needs 150 units of Tritanium. You can get this by refining the Veldspar you mined, or by mining more. Deliver the Tritanium to your agent to receive a new mining frigate.', '', ''),
(2013, 1013, 1, 'Afterburner Assembly',
 'Your agent wants you to deliver 2 Civilian Afterburner modules. You can manufacture these using a blueprint or buy them from the market. Use the industry interface to manufacture them if you have the blueprint.', '', ''),
(2014, 1014, 1, 'Bulk Mining',
 'Your agent needs 7000 units of Tritanium. This is a larger mining job - use your new Venture and its expanded cargo hold. Mine Veldspar and refine it into Tritanium, or mine other ores directly.', '', ''),
(2015, 1015, 1, 'Electronic Parts',
 'Take 1x Crates of Electronic Parts to Brybier I - Moon 20 - Freedom Extension Warehouse (0.6 system). Use the courier mission interface to pick up and deliver the cargo. Watch out for lowsec pirates.', '', ''),
(2016, 1016, 1, 'Cap Boosters',
 'Your agent needs 20 Cap Booster 25 charges. Manufacture these using a blueprint or buy from the market. Cap Boosters are used with Capacitor Booster modules to instantly restore capacitor.', '', ''),
(2017, 1017, 1, 'Courier Run',
 'Take 20 Cap Booster 25 to Vittenyn VI - Moon 13 - Federation Navy Assembly Plant (0.9). This inter-system courier run teaches you about traveling between stations.', '', ''),
(2018, 1018, 1, 'Shuttle Manufacturing',
 'Your agent wants you to deliver a Gallente Shuttle. Manufacture it using the Civilian Gallente Shuttle Blueprint you receive. This requires understanding manufacturing mechanics - install the job at a station with manufacturing facilities.', '', ''),
(2019, 1019, 1, 'Production Assistant',
 'Your agent needs a Production Assistant. This item can be manufactured or bought. Check the market or manufacture it yourself using the appropriate blueprint.', '', ''),
(2020, 1020, 1, 'Navitas Construction',
 'Your agent wants you to deliver a Navitas-class frigate. Manufacture it using the Navitas Blueprint. This requires significant Tritanium and other minerals. The Navitas is a solid Gallente mining frigate.', '', '');

INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(1011, 2011, 136000, 0, 0, 0),
(1012, 2012, 126000, 0, 0, 0),
(1013, 2013, 241000, 0, 0, 0),
(1014, 2014, 121000, 0, 0, 0),
(1015, 2015, 56000,  0, 0, 0),
(1016, 2016, 228000, 0, 0, 0),
(1017, 2017, 23000,  0, 0, 0),
(1018, 2018, 191000, 0, 0, 0),
(1019, 2019, 73000,  0, 0, 0),
(1020, 2020, 275000, 0, 0, 0);

-- ============================================================
-- Balancing the Books — Gallente Business (10 missions)
-- ============================================================
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`, `dataID`) VALUES
(1031, 'Business: Mission 1 - Data Delivery', 1032, 4, 0),
(1032, 'Business: Mission 2 - Find the Black Box', 1033, 4, 0),
(1033, 'Business: Mission 3 - Reprocessing', 1034, 4, 0),
(1034, 'Business: Mission 4 - Encrypted Data', 1035, 4, 0),
(1035, 'Business: Mission 5 - Courier Delivery', 1036, 4, 0),
(1036, 'Business: Mission 6 - Tracking Computer', 1037, 4, 0),
(1037, 'Business: Mission 7 - Datacore Recovery', 1038, 4, 0),
(1038, 'Business: Mission 8 - Central Core', 1039, 4, 0),
(1039, 'Business: Mission 9 - Afterburners', 1040, 4, 0),
(1040, 'Business: Mission 10 - Ammo Manufacturing', 0, 4, 0);

INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(2031, 1031, 1, 'Data Delivery',
 'Deliver Data Sheets to Vittenyn IV - Moon 6 - Expert Distribution Warehouse. Use the courier mission interface to pick up the cargo at your agent station and deliver it.', '', ''),
(2032, 1032, 1, 'Find the Black Box',
 'Travel to the Ainaille system (0.8) and find the Black Box. This item is in a wreck - use your Salvager module to extract it. The Serpentis pirates have been causing trouble in the area.', '', ''),
(2033, 1033, 1, 'Reprocessing',
 'Deliver 333 units of Tritanium. You can mine Veldspar and refine it at a station using a Reprocessing Plant. This teaches you the basics of mineral reprocessing.', '', ''),
(2034, 1034, 1, 'Encrypted Data',
 'Find the Encoded Data Chip in the Trossere system. This requires scanning - fit a Data Analyzer and scan down the data site. The Serpentis Corporation has hidden sensitive information.', '', ''),
(2035, 1035, 1, 'Courier Delivery',
 'Deliver the Encoded Data Chip to Brybier I - Moon 1 - Federal Freight Storage (0.6). Watch out for lowsec travel risks.', '', ''),
(2036, 1036, 1, 'Tracking Computer',
 'Deliver a Tracking Computer I module. You can buy this on the market or manufacture it. Tracking Computers improve your turret tracking speed and range.', '', ''),
(2037, 1037, 1, 'Datacore Recovery',
 'Find a Datacore - Elementary Civilian Tech in the Trossere system. Scan for relic sites and use your Relic Analyzer to hack the containers. Serpentis ruins hold valuable datacores.', '', ''),
(2038, 1038, 1, 'Central Core',
 'Deliver the Central Data Core to Vittenyn VI - Moon 13 - Federation Navy Assembly Plant. This teaches you about traveling between stations.', '', ''),
(2039, 1039, 1, 'Afterburners',
 'Deliver 2x 1MN Afterburner I modules. Manufacture them using blueprints or buy from the market. Afterburners increase your ship velocity.', '', ''),
(2040, 1040, 1, 'Ammo Manufacturing',
 'Deliver 5000 Antimatter Charge S rounds. Manufacture them using the Antimatter Charge S Blueprint. This requires minerals - you can mine what you need or buy from the market.', '', '');

INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(1031, 2031, 98000,  0, 0, 0),
(1032, 2032, 163000, 0, 0, 0),
(1033, 2033, 144000, 0, 0, 0),
(1034, 2034, 104000, 0, 0, 0),
(1035, 2035, 89000,  0, 0, 0),
(1036, 2036, 68000,  0, 0, 0),
(1037, 2037, 93000,  0, 0, 0),
(1038, 2038, 15000,  0, 0, 0),
(1039, 2039, 17000,  0, 0, 0),
(1040, 2040, 206000, 0, 0, 0);

-- ============================================================
-- Exploration — Gallente Exploration (5 missions)
-- ============================================================
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`, `dataID`) VALUES
(1041, 'Exploration: Mission 1 - Cosmic Anomalies', 1042, 5, 0),
(1042, 'Exploration: Mission 2 - Combat Scan', 1043, 5, 0),
(1043, 'Exploration: Mission 3 - Data Sites', 1044, 5, 0),
(1044, 'Exploration: Mission 4 - Relic Sites', 1045, 5, 0),
(1045, 'Exploration: Mission 5 - Final Report', 0, 5, 0);

INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(2041, 1041, 1, 'Cosmic Anomalies',
 'Find a cosmic anomaly using your onboard scanner. Open the scanner (Alt+P), look for anomalies in the system, and warp to one. Approach the anomaly beacon to get proof of discovery.', '', ''),
(2042, 1042, 1, 'Combat Scan',
 'Scan down a combat signature in the Trossere system. Use your probe scanner to find the site, warp to it, and eliminate the pirates. This demonstrates how exploration leads to combat.', '', ''),
(2043, 1043, 1, 'Data Sites',
 'Find a data signature in the Trossere system. Probe it down, warp to the site, and use your Data Analyzer to hack the containers. Retrieve the Proof of Discovery: Data to complete the mission.', '', ''),
(2044, 1044, 1, 'Relic Sites',
 'Find a relic signature in the Trossere system. Probe it down, warp to the site, and use your Relic Analyzer to salvage relic containers. Retrieve the Proof of Discovery: Relic.', '', ''),
(2045, 1045, 1, 'Final Report',
 'Return to your agent with the Proof of Discovery from all three site types. Your agent will reward you for completing the exploration career training.', '', '');

INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(1041, 2041, 133000, 0, 0, 0),
(1042, 2042, 137000, 0, 0, 0),
(1043, 2043, 107000, 0, 0, 0),
(1044, 2044, 98000,  0, 0, 0),
(1045, 2045, 215000, 0, 0, 0);

-- Link career agents to tutorial categories for the mapping service
-- (In addition to the tutorial_career_agents table)

-- ============================================================
-- Advanced Military — 10 missions (tutorialID 1051-1060)
-- ============================================================
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`, `dataID`) VALUES
(1051, 'AdvMil: Mission 1 - The Swap', 1052, 2, 0),
(1052, 'AdvMil: Mission 2 - Angel of Mercy', 1053, 2, 0),
(1053, 'AdvMil: Mission 3 - Your Undivided Attention', 1054, 2, 0),
(1054, 'AdvMil: Mission 4 - A Friend in Need', 1055, 2, 0),
(1055, 'AdvMil: Mission 5 - The Stand', 1056, 2, 0),
(1056, 'AdvMil: Mission 6 - Don\'t Look Back', 1057, 2, 0),
(1057, 'AdvMil: Mission 7 - Weapon of Choice', 1058, 2, 0),
(1058, 'AdvMil: Mission 8 - The Pacifist', 1059, 2, 0),
(1059, 'AdvMil: Mission 9 - Glue', 1060, 2, 0),
(1060, 'AdvMil: Mission 10 - The Exam', 0, 2, 0);

INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(2051, 1051, 1, 'The Swap',
 'Clear out a small gang of pirates to re-familiarize yourself with combat. This mission serves as a warm-up for the Advanced Military career path. Use your weapons and shield booster to defeat the pirates.', '', ''),
(2052, 1052, 1, 'Angel of Mercy',
 'Your agent has fitted a frigate with an explosive payload. Your mission is to fly this ship into a pirate starbase. The ship is on a one-way trip - make it count. Warp to the starbase and get as close as possible before detonation.', '', ''),
(2053, 1053, 1, 'Your Undivided Attention',
 'Use a Warp Disruptor on a fleeing pirate vessel to prevent it from warping away. The disruptor must be activated while the target is within range and locked. This teaches the importance of warp disruption in PvP.', '', ''),
(2054, 1054, 1, 'A Friend in Need',
 'Find the stranded agent\'s vessel and repair it using a Remote Armor Repairer or Shield Transporter. This teaches remote repair mechanics, essential for fleet support roles.', '', ''),
(2055, 1055, 1, 'The Stand',
 'Fight pirate ships until your ship is destroyed. Your agent has provided a disposable frigate for this mission. Do not warp out - fight until you lose your ship. This teaches you about pod recovery and ship loss.', '', ''),
(2056, 1056, 1, 'Don\'t Look Back',
 'Use an Afterburner to fly through a hazard cloud at maximum speed. The cloud damages ships that remain inside, so sustained Afterburner use is essential to pass through safely.', '', ''),
(2057, 1057, 1, 'Weapon of Choice',
 'Use the granted weapon to destroy an enemy ship. This mission teaches you to use racial weapon systems effectively. The weapon provided is appropriate for your race.', '', ''),
(2058, 1058, 1, 'The Pacifist',
 'Follow CONCORD\'s orders to introduce you to basic fleet operations. Do not fire on anyone - this mission is about following orders and understanding fleet hierarchy.', '', ''),
(2059, 1059, 1, 'Glue',
 'Use the granted Stasis Webifier on a pirate ship to slow it down. The webifier reduces the target\'s velocity, making it easier for you and your fleet to track and destroy it.', '', ''),
(2060, 1060, 1, 'The Exam',
 'Engage in combat with an enemy that uses stasis webifiers and warp scramblers. This final exam tests everything you\'ve learned. Destroy the enemy to complete the Advanced Military career path.', '', '');

INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(1051, 2051, 64000,  0, 0, 0),
(1052, 2052, 123000, 0, 0, 0),
(1053, 2053, 101000, 0, 0, 0),
(1054, 2054, 20000,  0, 0, 0),
(1055, 2055, 142000, 0, 0, 0),
(1056, 2056, 27000,  0, 0, 0),
(1057, 2057, 60000,  0, 0, 0),
(1058, 2058, 20000,  0, 0, 0),
(1059, 2059, 30000,  0, 0, 0),
(1060, 2060, 175000, 0, 0, 0);

-- +migrate Down
DELETE FROM `tutorial_rewards` WHERE `tutorialID` BETWEEN 1011 AND 1060;
DELETE FROM `tutorial_pages` WHERE `tutorialID` BETWEEN 1011 AND 1060;
DELETE FROM `tutorials` WHERE `tutorialID` BETWEEN 1011 AND 1060;
