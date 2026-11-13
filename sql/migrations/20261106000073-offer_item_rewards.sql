-- Give already-open standard offers an item reward too (new offers use the
-- deterministic fallback in MissionDataMgr::CreateMissionOffer).
-- +migrate Up
UPDATE agtOffers SET rewardItemID=2454, rewardItemQty=1
 WHERE rewardItemID=0 AND rewardItemQty=0
   AND typeID NOT IN (8,9,10);

-- +migrate Down
-- (existing reward rows are intentionally retained)
