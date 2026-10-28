-- +migrate Up
-- mktHistory had no unique key, so every price-history run appended the same
-- rows again (39M rows vs 379k unique on the live DB), and because the
-- mktUpdates row was missing the aggregate re-ran on EVERY server boot ("Starting
-- Market Manager" hanging for minutes). One row per (region,type,date), and the
-- seed row lets the 6h window actually work.
ALTER TABLE mktHistory ADD UNIQUE KEY IF NOT EXISTS uk_mkthistory (regionID, typeID, historyDate);
INSERT IGNORE INTO mktUpdates (server, timeStamp) VALUES (1, 0);

-- +migrate Down
ALTER TABLE mktHistory DROP INDEX uk_mkthistory;
