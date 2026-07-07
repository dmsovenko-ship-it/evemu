-- Add name/description columns to chrShipFittings
-- +migrate Up

ALTER TABLE chrShipFittings ADD COLUMN `fittingName` varchar(128) NOT NULL DEFAULT '' AFTER `shipDNA`;
ALTER TABLE chrShipFittings ADD COLUMN `description` varchar(512) NOT NULL DEFAULT '' AFTER `fittingName`;

-- +migrate Down
ALTER TABLE chrShipFittings DROP COLUMN `description`;
ALTER TABLE chrShipFittings DROP COLUMN `fittingName`;
