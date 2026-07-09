-- POS reactor data persistence
-- +migrate Up
CREATE TABLE IF NOT EXISTS `posReactorData` (
  `itemID` int(11) NOT NULL,
  `active` tinyint(1) NOT NULL DEFAULT 0,
  `reaction` smallint(6) NOT NULL DEFAULT 0,
  `connections` blob DEFAULT NULL,
  `demands` blob DEFAULT NULL,
  `supplies` blob DEFAULT NULL,
  PRIMARY KEY (`itemID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
-- +migrate Down
DROP TABLE IF EXISTS `posReactorData`;
