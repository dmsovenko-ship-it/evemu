-- Add reinforceHour column to mapSystemSovInfo for configurable IHub reinforcement exit
-- +migrate Up

ALTER TABLE mapSystemSovInfo
ADD COLUMN `reinforceHour` tinyint(4) NOT NULL DEFAULT 12 AFTER `contested`;

-- +migrate Down

ALTER TABLE mapSystemSovInfo
DROP COLUMN `reinforceHour`;
