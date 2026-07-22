-- Add index on ownerID for mktOrders (used by GetOrdersForOwner)
-- +migrate Up
ALTER TABLE mktOrders
    ADD INDEX IF NOT EXISTS idx_owner (ownerID);
-- +migrate Down
ALTER TABLE mktOrders
    DROP INDEX IF EXISTS idx_owner;
