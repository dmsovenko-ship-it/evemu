-- Fix incursion reward LP rows: rewardTypeID must be 1 (const.rewardTypeLP),
-- not 0 (const.rewardTypeISK). The client's GetMaxRewardValue filters tables
-- by rewardTypeID and OVERWRITES the running max per matching table — two rows
-- with rewardTypeID=0 caused the last (LP, quantity=0) to zero out the ISK max,
-- producing maxRewardValue=0 and a client ZeroDivisionError in DoRewardChart_Thread.
-- Also store the LP amount in rewardQuantity so entries[].quantity carries it
-- (GetMaxRewardValue reads e.quantity, not lpAmount).
-- +migrate Up

UPDATE `incursionRewards` SET `rewardTypeID` = 1, `rewardQuantity` = `lpAmount`
WHERE `lpAmount` > 0 AND `rewardQuantity` = 0;

-- +migrate Down
UPDATE `incursionRewards` SET `rewardTypeID` = 0, `rewardQuantity` = 0
WHERE `lpAmount` > 0 AND `rewardQuantity` = `lpAmount`;
