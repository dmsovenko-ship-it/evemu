-- +migrate Up
ALTER TABLE repStandings ADD COLUMN `lastModified` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `standing`;
UPDATE repStandings SET lastModified = UNIX_TIMESTAMP() * 10000000;

-- +migrate Down
ALTER TABLE repStandings DROP COLUMN `lastModified`;
