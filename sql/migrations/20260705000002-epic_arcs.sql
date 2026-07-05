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
REPLACE INTO `epicArc` (`arcID`, `arcName`, `factionID`, `description`, `cooldownDays`, `startingAgentID`, `startingSystemID`, `level`)
VALUES (1, 'The Blood-Stained Stars', 500020, 'Sisters of EVE Epic Arc - Investigate rogue drone activity across New Eden.', 90, 3019502, 30005217, 1);

-- Chapters
REPLACE INTO `epicArcChapter` (`arcID`, `chapterNumber`, `chapterName`) VALUES
(1, 1, 'Quality of Mercy'),
(1, 2, 'Automaton Impediment'),
(1, 3, 'Shadow Puppets'),
(1, 4, 'Queens And Drones'),
(1, 5, 'Shifting Foundations'),
(1, 6, 'A Breach of Trust'),
(1, 7, 'Closing In');

-- NOTE: epicArcMission data is now handled entirely by
-- 20260705000004-fix_epic_arc_pk.sql (supports branching via branchID in PK).



-- +migrate Down
DROP TABLE IF EXISTS `chrEpicArcState`;
DROP TABLE IF EXISTS `epicArcMission`;
DROP TABLE IF EXISTS `epicArcChapter`;
DROP TABLE IF EXISTS `epicArc`;
DELETE FROM `agtAgents` WHERE `agentID` = 3019502;
