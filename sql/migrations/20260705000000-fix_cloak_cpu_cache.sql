-- Covert Ops Cloaking Device II has cpuNeed=10000 by design, but the
-- EVE client blocks fitting client-side using the cache value before
-- the server can apply the ship's cpuNeedBonus (-99% on Black Ops).
-- Since canFitShipGroup1=898 restricts to Black Ops only, lower cpuNeed
-- to 0 so the client allows fitting. The server-side Online() check
-- also applies cpuNeedBonus for correct validation.
-- +migrate Up
UPDATE dgmTypeAttributes
SET valueInt = 0
WHERE typeID = 11578 AND attributeID = 50;

-- +migrate Down
UPDATE dgmTypeAttributes
SET valueInt = 10000
WHERE typeID = 11578 AND attributeID = 50;
