-- Corporation market tab: both sides of the self-join need owner+bid+type.
-- +migrate Up
ALTER TABLE mktOrders ADD INDEX idx_owner_bid_type (ownerID, bid, typeID);

-- +migrate Down
ALTER TABLE mktOrders DROP INDEX idx_owner_bid_type;
