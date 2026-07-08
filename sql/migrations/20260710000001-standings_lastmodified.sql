-- Add lastModified column to repStandings for decay tracking
ALTER TABLE repStandings ADD COLUMN `lastModified` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `standing`;
-- Set initial lastModified to current time for all existing rows
UPDATE repStandings SET lastModified = UNIX_TIMESTAMP() * 10000000;
