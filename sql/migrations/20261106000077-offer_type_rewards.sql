-- Profession-appropriate fallback reward items for already-open offers.
-- +migrate Up
UPDATE agtOffers SET rewardItemID=483, rewardItemQty=1 WHERE typeID=5 AND rewardItemID=2454;
UPDATE agtOffers SET rewardItemID=1317, rewardItemQty=1 WHERE typeID IN (3,4) AND rewardItemID=2454;

-- +migrate Down
-- retained
