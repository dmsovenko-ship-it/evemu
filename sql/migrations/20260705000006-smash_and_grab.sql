-- Smash and Grab (Guristas Pirates Epic Arc) — Crucible-era, level 3
-- 12 missions, 3 chapters, 3-start branch + Irichi/Kori branch
-- +migrate Up

-- Epic arc agents
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019507, 24, 1000166, 60015139, 3, 20, 10, 0),  -- Arment Caute (Gallente path)
(3019508, 24, 1000166, 60015140, 3, 20, 10, 0),  -- Atma Aulato (Caldari path)
(3019509, 24, 1000166, 60015141, 3, 20, 10, 0);  -- Yada Vinjivas (Guristas path)

-- agtMissions
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80077, 0, 'Enemy of my Enemy', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80078, 0, 'Turning Coat', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80079, 0, 'Recruitment Drive', 3, 10, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(80080, 0, 'Intelligence Mining', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80081, 0, 'Planning the Operation', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80082, 0, 'Sabotage 101', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80083, 0, 'Brassy Faced Bastard', 3, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80084, 0, 'Upward Momentum', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80085, 0, 'Miscommunication', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80086, 0, 'Fuel Gauge', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80087, 0, 'Knockout Punch', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80088, 0, 'Culling the Weak', 3, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80089, 0, 'Threat Assessment', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80090, 0, 'Dread Pirates', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80091, 0, 'Rabbit Hole', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80092, 0, 'Passing the Buck', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80093, 0, 'Smoke and Mirrors', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80094, 0, 'Foxfire', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80095, 0, 'Spy Games', 3, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0);

-- Epic arc definition
REPLACE INTO `epicArc` (`arcID`, `arcName`, `factionID`, `description`, `cooldownDays`, `startingAgentID`, `startingSystemID`, `level`)
VALUES (4, 'Smash and Grab', 500014, 'Guristas Pirates Epic Arc — Nullsec operations in Venal region.', 90, 3019507, 30005217, 3);

-- Chapters
REPLACE INTO `epicArcChapter` (`arcID`, `chapterNumber`, `chapterName`) VALUES
(4, 1, 'Probation'),
(4, 2, 'For Fun and Profit'),
(4, 3, 'Internal Security');

-- Missions with branching
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
-- Chapter 1 (branch: 1=Gallente, 2=Caldari, 3=Guristas)
(4, 1, 1, 80077, 'Enemy of my Enemy', 1, 250000, 0),
(4, 1, 1, 80078, 'Turning Coat', 2, 250000, 0),
(4, 1, 1, 80079, 'Recruitment Drive', 3, 250000, 0),
(4, 1, 2, 80080, 'Intelligence Mining', 0, 300000, 0),
(4, 1, 3, 80081, 'Planning the Operation', 0, 300000, 0),
(4, 1, 4, 80082, 'Sabotage 101', 1, 300000, 0),
(4, 1, 4, 80083, 'Brassy Faced Bastard', 2, 300000, 0),
(4, 1, 5, 80084, 'Upward Momentum', 0, 350000, 0),
-- Chapter 2 (no branch)
(4, 2, 6, 80085, 'Miscommunication', 0, 350000, 0),
(4, 2, 7, 80086, 'Fuel Gauge', 0, 350000, 0),
(4, 2, 8, 80087, 'Knockout Punch', 0, 400000, 0),
(4, 2, 9, 80088, 'Culling the Weak', 0, 350000, 0),
-- Chapter 3 (branch: 1=Irichi, 2=Kori)
(4, 3, 10, 80089, 'Threat Assessment', 1, 400000, 0),
(4, 3, 11, 80090, 'Dread Pirates', 1, 400000, 0),
(4, 3, 12, 80091, 'Rabbit Hole', 1, 400000, 0),
(4, 3, 13, 80092, 'Passing the Buck', 1, 400000, 0),
(4, 3, 10, 80093, 'Smoke and Mirrors', 2, 400000, 0),
(4, 3, 11, 80094, 'Foxfire', 2, 400000, 0),
(4, 3, 12, 80095, 'Spy Games', 2, 400000, 0);

-- +migrate Down
DELETE FROM `epicArcMission` WHERE `arcID` = 4;
DELETE FROM `epicArcChapter` WHERE `arcID` = 4;
DELETE FROM `epicArc` WHERE `arcID` = 4;
DELETE FROM `agtMissions` WHERE `id` BETWEEN 80077 AND 80095;
DELETE FROM `agtAgents` WHERE `agentID` IN (3019507, 3019508, 3019509);
