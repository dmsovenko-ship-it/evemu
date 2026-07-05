-- Angel Sound (Angel Cartel Epic Arc) — Crucible-era content, level 3
-- 11 missions, 3 chapters, branching start + Chapter 2 branch
-- +migrate Up

-- Epic arc agents (agentTypeID=10 = EpicArcAgent)
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019504, 24, 1000167, 60015080, 3, 20, 10, 0),  -- Aton Hordner (Minmatar path)
(3019505, 24, 1000168, 60015093, 3, 20, 10, 0),  -- Arajna Ashia (Amarr path)
(3019506, 24, 1000157, 60015132, 3, 20, 10, 0);  -- Ellar Stin (Angel path)

-- agtMissions entries
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80058, 0, 'The Balance of Power', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80059, 0, 'Mistaken Identity', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80060, 0, 'Headhunted', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80061, 0, 'New Opportunities', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80062, 0, 'Fight or Flight', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80063, 0, 'Serpentis Fill-In', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80064, 0, 'Ride to the Rescue', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80065, 0, 'The Best Kind of Revenge', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80066, 0, 'Wrath of Angels', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80067, 0, 'Natural Consequences', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80068, 0, 'Clear the Way', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80069, 0, 'Salvage Heist', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80070, 0, 'Rabbit Trap', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80071, 0, 'Dominus', 3, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80072, 0, 'The Lesser of Two', 3, 10, 0, 0, 0, 0, 0, 0, 450000, 0, 0, 90000, 0),
(80073, 0, 'Situation Normal', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80074, 0, 'Breaking the Lock', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80075, 0, 'Data Destruction', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80076, 0, 'Fear of Angels', 3, 10, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 200000, 0);

-- Epic arc definition (3 agents, multi-start)
REPLACE INTO `epicArc` (`arcID`, `arcName`, `factionID`, `description`, `cooldownDays`, `startingAgentID`, `startingSystemID`, `level`)
VALUES (3, 'Angel Sound', 500011, 'Angel Cartel Epic Arc — Nullsec operations in Curse region.', 90, 3019504, 30005217, 3);

-- Chapters
REPLACE INTO `epicArcChapter` (`arcID`, `chapterNumber`, `chapterName`) VALUES
(3, 1, 'Rapture'),
(3, 2, 'Heaven'),
(3, 3, 'Paradise');

-- Missions with branching
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
-- Chapter 1: Rapture (branch at start: 1=Minmatar, 2=Amarr, 3=Angel)
(3, 1, 1, 80058, 'The Balance of Power', 1, 250000, 0),
(3, 1, 1, 80059, 'Mistaken Identity', 2, 250000, 0),
(3, 1, 1, 80060, 'Headhunted', 3, 250000, 0),
-- Merge after first mission
(3, 1, 2, 80061, 'New Opportunities', 0, 300000, 0),
(3, 1, 3, 80062, 'Fight or Flight', 0, 300000, 0),
-- Chapter 2 (branch: 1=Heaven, 2=Utopia)
(3, 2, 4, 80063, 'Serpentis Fill-In', 1, 350000, 0),
(3, 2, 5, 80064, 'Ride to the Rescue', 1, 350000, 0),
(3, 2, 6, 80065, 'The Best Kind of Revenge', 1, 350000, 0),
(3, 2, 7, 80066, 'Wrath of Angels', 1, 400000, 0),
(3, 2, 4, 80067, 'Natural Consequences', 2, 350000, 0),
(3, 2, 5, 80068, 'Clear the Way', 2, 350000, 0),
(3, 2, 6, 80069, 'Salvage Heist', 2, 350000, 0),
(3, 2, 7, 80070, 'Rabbit Trap', 2, 350000, 0),
-- Merge
(3, 2, 8, 80071, 'Dominus', 0, 500000, 0),
(3, 2, 9, 80072, 'The Lesser of Two', 0, 450000, 0),
-- Chapter 3: Paradise (branch: 1a/2a vs b)
(3, 3, 10, 80073, 'Situation Normal', 1, 400000, 0),
(3, 3, 10, 80074, 'Breaking the Lock', 1, 400000, 0),
(3, 3, 10, 80075, 'Data Destruction', 2, 400000, 0),
-- Merge to final
(3, 3, 11, 80076, 'Fear of Angels', 0, 1000000, 0.3);

-- +migrate Down
DELETE FROM `epicArcMission` WHERE `arcID` = 3;
DELETE FROM `epicArcChapter` WHERE `arcID` = 3;
DELETE FROM `epicArc` WHERE `arcID` = 3;
DELETE FROM `agtMissions` WHERE `id` BETWEEN 80058 AND 80076;
DELETE FROM `agtAgents` WHERE `agentID` IN (3019504, 3019505, 3019506);
