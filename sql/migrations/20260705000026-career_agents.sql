-- Career agents for all 4 races (3 per race: Industry, Business, Exploration)
-- agentTypeID=3 (Tutorial), divisionID=1 (Security), quality=20
-- Each race has career agents at 3 school corporations in starter systems
-- +migrate Up

CREATE TABLE IF NOT EXISTS `tutorial_career_agents` (
  `raceID` tinyint(4) NOT NULL DEFAULT 0,
  `careerType` varchar(32) NOT NULL DEFAULT '',
  `agentID` int(11) NOT NULL DEFAULT 0,
  `stationID` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`raceID`,`careerType`),
  KEY `agentID` (`agentID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Amarr Empire (raceID=4) — Hedion University, Royal Amarr Institute, Imperial Academy
REPLACE INTO `tutorial_career_agents` (`raceID`, `careerType`, `agentID`, `stationID`) VALUES
(4, 'Industry',     3019800, 60015001),
(4, 'Business',     3019801, 60015001),
(4, 'Exploration',  3019802, 60015001);

-- Caldari State (raceID=1) — School of Applied Knowledge, State War Academy, Science and Trade Institute
REPLACE INTO `tutorial_career_agents` (`raceID`, `careerType`, `agentID`, `stationID`) VALUES
(1, 'Industry',     3019803, 60015005),
(1, 'Business',     3019804, 60015005),
(1, 'Exploration',  3019805, 60015005);

-- Gallente Federation (raceID=8) — Center for Advanced Studies, Federal Navy Academy, University of Caille
REPLACE INTO `tutorial_career_agents` (`raceID`, `careerType`, `agentID`, `stationID`) VALUES
(8, 'Industry',     3019806, 60015010),
(8, 'Business',     3019807, 60015010),
(8, 'Exploration',  3019808, 60015010);

-- Minmatar Republic (raceID=2) — Republic University, Republic Military School, Pator Tech School
REPLACE INTO `tutorial_career_agents` (`raceID`, `careerType`, `agentID`, `stationID`) VALUES
(2, 'Industry',     3019809, 60015016),
(2, 'Business',     3019810, 60015016),
(2, 'Exploration',  3019811, 60015016);

-- Career agents as tutorial agents
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
-- Amarr (raceID=4, school corpID)
(3019800, 1, 1000075, 60015001, 1, 20, 3, 0),
(3019801, 1, 1000075, 60015001, 1, 20, 3, 0),
(3019802, 1, 1000075, 60015001, 1, 20, 3, 0),
-- Caldari (raceID=1, school corpID)
(3019803, 1, 1000168, 60015005, 1, 20, 3, 0),
(3019804, 1, 1000168, 60015005, 1, 20, 3, 0),
(3019805, 1, 1000168, 60015005, 1, 20, 3, 0),
-- Gallente (raceID=8, school corpID)
(3019806, 1, 1000169, 60015010, 1, 20, 3, 0),
(3019807, 1, 1000169, 60015010, 1, 20, 3, 0),
(3019808, 1, 1000169, 60015010, 1, 20, 3, 0),
-- Minmatar (raceID=2, school corpID)
(3019809, 1, 1000170, 60015016, 1, 20, 3, 0),
(3019810, 1, 1000170, 60015016, 1, 20, 3, 0),
(3019811, 1, 1000170, 60015016, 1, 20, 3, 0);

-- +migrate Down
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019800 AND 3019811;
DROP TABLE IF EXISTS `tutorial_career_agents`;
