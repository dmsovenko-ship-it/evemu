-- Add index on ownerID for mktOrders (used by GetOrdersForOwner)
ALTER TABLE mktOrders
    ADD INDEX idx_owner (ownerID);
