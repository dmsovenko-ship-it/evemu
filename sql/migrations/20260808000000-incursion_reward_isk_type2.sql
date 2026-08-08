-- Fix incursion ISK reward rows: rewardTypeID must be 2 (const.rewardTypeISK),
-- not 0. Verified against the live client: the journal's GetMaxRewardValue filters
-- reward tables by const.rewardTypeISK, which equals 2 in this client build
-- (LP rows use rewardTypeID=1 = const.rewardTypeLP and display correctly, while
-- ISK rows with rewardTypeID=0 never matched and showed 0 ISK).
-- +migrate Up

UPDATE `incursionRewards` SET `rewardTypeID` = 2
WHERE `lpAmount` = 0 AND `lpTypeID` = 0 AND `rewardTypeID` = 0;

-- +migrate Down
UPDATE `incursionRewards` SET `rewardTypeID` = 0
WHERE `lpAmount` = 0 AND `lpTypeID` = 0 AND `rewardTypeID` = 2;
