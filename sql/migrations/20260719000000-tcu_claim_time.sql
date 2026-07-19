-- Persist TCU claim timer so sovereignty claim completes even after system unload
-- +migrate Up

ALTER TABLE posStructureData
ADD COLUMN `claimTime` bigint(20) NOT NULL DEFAULT 0 AFTER `timestamp`;

-- +migrate Down

ALTER TABLE posStructureData
DROP COLUMN `claimTime`;
