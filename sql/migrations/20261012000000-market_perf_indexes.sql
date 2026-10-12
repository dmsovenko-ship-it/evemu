-- +migrate Up
-- Portal / market hot-path indexes.
--
-- mktOrders is the biggest table (38M rows, ~9 GB): the portal's bulk queries
-- (top valuables by type, best price per type in a region, market stats) filter
-- by typeID + regionID and sort by price. The single-column typeID index does
-- not help once a region filter is present, so add a covering composite.
CREATE INDEX IF NOT EXISTS idx_mktorders_region_type ON mktOrders (regionID, typeID, bid, price);
-- idx_region_type_bid (regionID, typeID, bid) from an earlier migration is a
-- strict prefix of the composite above - drop it to avoid index bloat on the
-- 38M-row table.
DROP INDEX IF EXISTS idx_region_type_bid ON mktOrders;
-- Sell/buy book lookups by station + type (station order lists, best ask/bid).
CREATE INDEX IF NOT EXISTS idx_mktorders_station_type ON mktOrders (stationID, typeID, bid);

-- mktHistory pruning / stats by type+region over time.
CREATE INDEX IF NOT EXISTS idx_mkthistory_type_date ON mktHistory (typeID, regionID, historyDate);

-- +migrate Down
DROP INDEX IF EXISTS idx_mktorders_region_type ON mktOrders;
CREATE INDEX IF NOT EXISTS idx_region_type_bid ON mktOrders (regionID, typeID, bid);
DROP INDEX IF EXISTS idx_mktorders_station_type ON mktOrders;
DROP INDEX IF EXISTS idx_mkthistory_type_date ON mktHistory;
