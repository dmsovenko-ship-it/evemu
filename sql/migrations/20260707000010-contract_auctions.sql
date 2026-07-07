-- Auction contract support: bids table
-- +migrate Up

CREATE TABLE IF NOT EXISTS `ctrBids` (
  `bidID` int(11) NOT NULL AUTO_INCREMENT,
  `contractID` int(11) NOT NULL,
  `bidderID` int(11) NOT NULL,
  `amount` double NOT NULL DEFAULT 0,
  `timeBid` bigint(20) NOT NULL DEFAULT 0,
  `isCorp` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`bidID`),
  KEY `contractID` (`contractID`),
  KEY `bidderID` (`bidderID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS `ctrBids`;
