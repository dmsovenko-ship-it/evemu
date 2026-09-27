-- +migrate Up
-- Station management: the corp "UpdateStationManagementSettings" call writes the
-- per-service ACCESS RULES and COST MODIFIERS, but the tables were never created
-- (the writes failed silently). Create them.
CREATE TABLE IF NOT EXISTS `staStationServiceAccessRules` (
  `stationID`   int(10) unsigned NOT NULL,
  `serviceID`   int(10) unsigned NOT NULL,
  `accessGroup` int(11) NOT NULL DEFAULT 0,
  `newValue`    int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`stationID`,`serviceID`,`accessGroup`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `staStationServiceCostModifiers` (
  `stationID`   int(10) unsigned NOT NULL,
  `serviceID`   int(10) unsigned NOT NULL,
  `costModifier` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`stationID`,`serviceID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `staStationServiceCostModifiers`;
DROP TABLE IF EXISTS `staStationServiceAccessRules`;
