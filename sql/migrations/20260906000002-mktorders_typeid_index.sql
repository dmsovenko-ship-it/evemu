-- Speed up the portal's ISK valuation queries that run over the full mktOrders
-- table (38.5M rows):
--   * TopValuables:   SELECT typeID, AVG(price) ... WHERE typeID IN (...) GROUP BY typeID
--   * Resolve:        PERCENTILE_CONT ... OVER (PARTITION BY typeID) FROM mktOrders WHERE typeID = t.typeID
-- Neither could use idx_region_type_bid (regionID,typeID,bid) because typeID is
-- not the leading column and no regionID is supplied -> full scans made the
-- homepage / kill pages hang on a cold first load.
-- +migrate Up
ALTER TABLE mktOrders
    ADD INDEX IF NOT EXISTS idx_type (typeID);

-- +migrate Down
ALTER TABLE mktOrders
    DROP INDEX IF EXISTS idx_type;
