-- +migrate Up
-- Market hot-path indexes.
-- GetRegionBest (region price list) was doing a full temp-table GROUP BY over the
-- whole region (~3M orders): 11.8s -> 33ms after the (regionID,bid,typeID,price)
-- index. Station/system asks and Find*Order get matching covering indexes too.
-- Redundant single-column indexes are dropped to keep bot write cost down.
ALTER TABLE `mktOrders`
  ADD INDEX `idx_mkt_region_bid_type_price` (`regionID`, `bid`, `typeID`, `price`),
  ADD INDEX `idx_mkt_station_type_price` (`stationID`, `typeID`, `price`),
  ADD INDEX `idx_mkt_station_type_bid_price` (`stationID`, `typeID`, `bid`, `price`),
  ADD INDEX `idx_mkt_system_type_price` (`solarSystemID`, `typeID`, `price`),
  DROP INDEX `typeID`,
  DROP INDEX `regionID`,
  DROP INDEX `stationID`,
  DROP INDEX `idx_mktorders_station_type`;

-- +migrate Down
ALTER TABLE `mktOrders`
  ADD INDEX `typeID` (`typeID`),
  ADD INDEX `regionID` (`regionID`),
  ADD INDEX `stationID` (`stationID`),
  ADD INDEX `idx_mktorders_station_type` (`stationID`, `typeID`, `bid`),
  DROP INDEX `idx_mkt_region_bid_type_price`,
  DROP INDEX `idx_mkt_station_type_price`,
  DROP INDEX `idx_mkt_station_type_bid_price`,
  DROP INDEX `idx_mkt_system_type_price`;
