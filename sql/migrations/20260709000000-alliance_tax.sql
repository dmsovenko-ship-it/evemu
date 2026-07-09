-- Adds taxRate column to alnAlliance for alliance tax support
-- +migrate Up
ALTER TABLE alnAlliance
ADD COLUMN `taxRate` decimal(5,4) NOT NULL DEFAULT 0.0000 AFTER url;
-- +migrate Down
ALTER TABLE alnAlliance
DROP COLUMN `taxRate`;
