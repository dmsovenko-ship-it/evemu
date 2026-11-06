-- +migrate Up
-- The tutorial chain view (GetTutorialsAndConnections) reads this table; the
-- table was never seeded by the SDE import, so the chain graph arrived empty.
-- Derive the connections from tutorials.nextTutorialID for every race (0 = all).
CREATE TABLE IF NOT EXISTS `tutorial_connections` (
  `tutorialID` int(10) unsigned NOT NULL,
  `raceID` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `nextTutorialID` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`tutorialID`, `raceID`, `nextTutorialID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT IGNORE INTO `tutorial_connections` (`tutorialID`, `raceID`, `nextTutorialID`)
  SELECT `tutorialID`, 0, `nextTutorialID` FROM `tutorials` WHERE `nextTutorialID` IS NOT NULL;

-- +migrate Down
DROP TABLE IF EXISTS `tutorial_connections`;
