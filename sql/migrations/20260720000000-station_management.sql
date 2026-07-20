-- Add columns and tables for Station Management settings
-- +migrate Up

ALTER TABLE staStations
  ADD COLUMN `description` text AFTER stationName,
  ADD COLUMN `exitTime` int(11) NOT NULL DEFAULT 0 AFTER reprocessingHangarFlag,
  ADD COLUMN `standingOwnerID` int(11) NOT NULL DEFAULT 0 AFTER exitTime;

CREATE TABLE IF NOT EXISTS staStationRentableItems (
  stationID int(11) NOT NULL,
  typeID int(11) NOT NULL,
  PRIMARY KEY (stationID, typeID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- +migrate Down

ALTER TABLE staStations
  DROP COLUMN `description`,
  DROP COLUMN `exitTime`,
  DROP COLUMN `standingOwnerID`;

DROP TABLE IF EXISTS staStationRentableItems;
