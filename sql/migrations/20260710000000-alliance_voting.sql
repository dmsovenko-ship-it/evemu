-- Alliance voting system (executor election, general votes)
-- +migrate Up
CREATE TABLE IF NOT EXISTS `alnVoteItems` (
  `voteCaseID` int(11) NOT NULL AUTO_INCREMENT,
  `allianceID` int(10) unsigned NOT NULL,
  `voteType` int(10) unsigned NOT NULL DEFAULT 0,
  `voteCaseText` varchar(100) NOT NULL DEFAULT 'No Label',
  `description` varchar(200) NOT NULL DEFAULT 'No Description',
  `inEffect` tinyint(1) unsigned NOT NULL DEFAULT 1,
  `status` tinyint(2) NOT NULL DEFAULT 2,
  `startDateTime` bigint(20) NOT NULL,
  `endDateTime` bigint(20) NOT NULL DEFAULT 0,
  `actedUpon` tinyint(1) unsigned NOT NULL DEFAULT 0,
  `timeActedUpon` bigint(20) NOT NULL DEFAULT 0,
  `rescended` tinyint(1) unsigned NOT NULL DEFAULT 0,
  `timeRescended` bigint(20) NOT NULL DEFAULT 0,
  `votesMade` smallint(5) NOT NULL DEFAULT 0,
  `votesProxied` smallint(5) NOT NULL DEFAULT 0,
  PRIMARY KEY (`voteCaseID`),
  KEY `allianceID` (`allianceID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `alnVoteOptions` (
  `ai` int(11) NOT NULL AUTO_INCREMENT,
  `voteCaseID` int(11) NOT NULL DEFAULT 0,
  `optionID` tinyint(2) DEFAULT NULL,
  `optionText` varchar(150) NOT NULL DEFAULT '',
  `parameter` int(10) DEFAULT NULL,
  `parameter1` int(10) DEFAULT NULL,
  `parameter2` int(10) DEFAULT NULL,
  `votesFor` int(10) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ai`),
  KEY `voteCaseID` (`voteCaseID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `alnVotes` (
  `corpID` int(11) NOT NULL,
  `allianceID` int(11) NOT NULL,
  `voteCaseID` mediumint(11) NOT NULL,
  `optionID` tinyint(1) NOT NULL,
  PRIMARY KEY (`corpID`, `voteCaseID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
-- +migrate Down
DROP TABLE IF EXISTS `alnVotes`;
DROP TABLE IF EXISTS `alnVoteOptions`;
DROP TABLE IF EXISTS `alnVoteItems`;
