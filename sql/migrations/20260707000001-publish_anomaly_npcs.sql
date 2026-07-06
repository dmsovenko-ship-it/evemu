-- Publish custom anomaly NPC types so the client loads them into cfg.invtypes
-- Without published=1, the EVE client filters them out and throws RecordNotFound
-- +migrate Up
UPDATE invTypes SET published = 1 WHERE typeID BETWEEN 33001 AND 33103;

-- +migrate Down
UPDATE invTypes SET published = 0 WHERE typeID BETWEEN 33001 AND 33103;
