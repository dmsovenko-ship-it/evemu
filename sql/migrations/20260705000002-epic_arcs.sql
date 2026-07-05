-- Epic Arc system: The Blood-Stained Stars (Sisters of EVE)
-- +migrate Up

CREATE TABLE IF NOT EXISTS `epicArc` (
  `arcID` int(5) NOT NULL DEFAULT 0,
  `arcName` varchar(100) NOT NULL DEFAULT '',
  `factionID` int(10) NOT NULL DEFAULT 0,
  `description` text DEFAULT NULL,
  `cooldownDays` int(5) NOT NULL DEFAULT 90,
  `startingAgentID` int(10) NOT NULL DEFAULT 0,
  `startingSystemID` int(10) NOT NULL DEFAULT 0,
  `level` tinyint(1) NOT NULL DEFAULT 1,
  PRIMARY KEY (`arcID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `epicArcChapter` (
  `arcID` int(5) NOT NULL DEFAULT 0,
  `chapterNumber` tinyint(2) NOT NULL DEFAULT 0,
  `chapterName` varchar(100) NOT NULL DEFAULT '',
  PRIMARY KEY (`arcID`, `chapterNumber`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `epicArcMission` (
  `arcID` int(5) NOT NULL DEFAULT 0,
  `chapterNumber` tinyint(2) NOT NULL DEFAULT 0,
  `sequenceNumber` tinyint(2) NOT NULL DEFAULT 0,
  `missionID` int(10) NOT NULL DEFAULT 0,
  `missionName` varchar(100) NOT NULL DEFAULT '',
  `branchID` tinyint(1) NOT NULL DEFAULT 0,
  `rewardISK` int(10) NOT NULL DEFAULT 0,
  `rewardStanding` float NOT NULL DEFAULT 0,
  PRIMARY KEY (`arcID`, `chapterNumber`, `sequenceNumber`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `chrEpicArcState` (
  `characterID` int(10) NOT NULL DEFAULT 0,
  `arcID` int(5) NOT NULL DEFAULT 0,
  `chapterNumber` tinyint(2) NOT NULL DEFAULT 0,
  `lastMissionID` int(10) NOT NULL DEFAULT 0,
  `branchChoice` tinyint(1) NOT NULL DEFAULT 0,
  `dateStarted` bigint(20) NOT NULL DEFAULT 0,
  `dateCompleted` bigint(20) NOT NULL DEFAULT 0,
  `completed` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`characterID`, `arcID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Sister Alitura - epic arc starting agent
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`)
VALUES (3019502, 24, 1000179, 60015054, 1, 20, 10, 0);

-- The Blood-Stained Stars (Epic Arc)
INSERT INTO `epicArc` (`arcID`, `arcName`, `factionID`, `description`, `cooldownDays`, `startingAgentID`, `startingSystemID`, `level`)
VALUES (1, 'The Blood-Stained Stars', 500020, 'Sisters of EVE Epic Arc - Investigate rogue drone activity across New Eden.', 90, 3019502, 30005217, 1);

-- Chapters
INSERT INTO `epicArcChapter` (`arcID`, `chapterNumber`, `chapterName`) VALUES
(1, 1, 'Quality of Mercy'),
(1, 2, 'Automaton Impediment'),
(1, 3, 'Shadow Puppets'),
(1, 4, 'Queens And Drones'),
(1, 5, 'Shifting Foundations'),
(1, 6, 'A Breach of Trust'),
(1, 7, 'Closing In');

-- Missions (Chapter 1: Quality of Mercy)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 1, 1, 0, 'A Beacon Beckons', 0, 15000, 0),
(1, 1, 2, 0, 'Agent Inquiry', 0, 7000, 0),
(1, 1, 3, 0, 'Of Interest', 0, 7500, 0),
(1, 1, 4, 0, 'Retrieving Red', 0, 7500, 0),
(1, 1, 5, 0, 'Alerting Alitura', 0, 7500, 0),
(1, 1, 6, 0, 'Jet-Canning a Janitor', 0, 15000, 0),
(1, 1, 7, 0, 'Chivvying a Chef', 0, 7500, 0),
(1, 1, 8, 0, 'Delivering a Doctor', 0, 7500, 0),
(1, 1, 9, 0, 'Engineering a Rescue', 0, 15000, 0),
(1, 1, 10, 0, 'Going Gallente', 0, 7500, 0);

-- Missions (Chapter 2: Automaton Impediment)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 2, 11, 0, 'Studying the Scene', 0, 7500, 0),
(1, 2, 12, 0, 'Rendering Assistance', 0, 7500, 0),
(1, 2, 13, 0, 'Lair of the Snakes', 0, 15000, 0),
(1, 2, 14, 0, 'Data Retrieval', 0, 7500, 0),
(1, 2, 15, 0, 'Crossing Enemy Lines', 0, 15000, 0);

-- Missions (Chapter 3: Shadow Puppets)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 3, 16, 0, 'Passive Observation', 0, 7500, 0),
(1, 3, 17, 0, 'House of Records', 0, 7500, 0),
(1, 3, 18, 0, 'Mercenary Distractions', 0, 15000, 0),
(1, 3, 19, 0, 'An Economy Under Threat', 0, 15000, 0),
(1, 3, 20, 0, 'Every Drone Inside', 0, 15000, 0),
(1, 3, 21, 0, 'A Sense of Dread', 0, 7500, 0);

-- Missions (Chapter 4: Queens And Drones - has branching)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 4, 22, 0, 'Royal Jelly', 0, 7500, 0),
(1, 4, 23, 0, 'Nature Pictures', 0, 7500, 0),
(1, 4, 24, 0, 'Tracking or Scanning', 0, 7500, 0),
(1, 4, 25, 0, 'Tracking the Queen (Part 1)', 1, 15000, 0),
(1, 4, 26, 0, 'Tracking the Queen (Part 2)', 1, 15000, 0),
(1, 4, 27, 0, 'Tracking the Queen (Part 3)', 1, 15000, 0),
(1, 4, 25, 0, 'Bag of Blood', 2, 15000, 0),
(1, 4, 26, 0, 'Planting the Body', 2, 15000, 0),
(1, 4, 27, 0, 'Chasing a Nightmare', 2, 15000, 0),
(1, 4, 28, 0, 'Burning Down the Hive', 0, 15000, 0),
(1, 4, 29, 0, 'It''s Not Over Yet', 0, 7500, 0);

-- Missions (Chapter 5: Shifting Foundations)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 5, 30, 0, 'An Eye on Everything', 0, 7500, 0),
(1, 5, 31, 0, 'The Uses of Force', 0, 15000, 0),
(1, 5, 32, 0, 'Goading the Leader', 0, 15000, 0),
(1, 5, 33, 0, 'Hunting the Lieutenants', 0, 15000, 0),
(1, 5, 34, 0, 'Valuable Cargo', 0, 15000, 0),
(1, 5, 35, 0, 'Marked for Death', 0, 15000, 0),
(1, 5, 36, 0, 'Thwarting the Succession', 0, 15000, 0),
(1, 5, 37, 0, 'Certificate of Death', 0, 15000, 0);

-- Missions (Chapter 6: A Breach of Trust)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 6, 38, 0, 'A Matter of Decorum', 0, 7500, 0),
(1, 6, 39, 0, 'New Friends', 0, 7500, 0),
(1, 6, 40, 0, 'Recovery', 0, 15000, 0),
(1, 6, 41, 0, 'Of Quiet Nights Long Past', 0, 7500, 0),
(1, 6, 42, 0, 'Revelations', 0, 7500, 0),
(1, 6, 43, 0, 'A Call to Trial', 0, 15000, 0),
(1, 6, 44, 0, 'Brothers and Sisters', 0, 15000, 0);

-- Missions (Chapter 7: Closing In)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 7, 45, 0, 'A Stranger''s Face', 0, 7500, 0),
(1, 7, 46, 0, 'The Sisters and the Spy', 0, 7500, 0),
(1, 7, 47, 0, 'Sealing the Deal', 0, 7500, 0),
(1, 7, 48, 0, 'Chasing Shadows', 0, 7500, 0),
(1, 7, 49, 0, 'The Missing Piece', 0, 7500, 0),
(1, 7, 50, 0, 'The Amarr Commander', 1, 15000, 0),
(1, 7, 50, 0, 'The Caldari Commander', 2, 15000, 0),
(1, 7, 50, 0, 'The Gallente Commander', 3, 15000, 0),
(1, 7, 50, 0, 'The Minmatar Commander', 4, 15000, 0),
(1, 7, 51, 0, 'Our Man Dagan', 0, 50000, 0.7);

-- +migrate Down
DROP TABLE IF EXISTS `chrEpicArcState`;
DROP TABLE IF EXISTS `epicArcMission`;
DROP TABLE IF EXISTS `epicArcChapter`;
DROP TABLE IF EXISTS `epicArc`;
DELETE FROM `agtAgents` WHERE `agentID` = 3019502;
