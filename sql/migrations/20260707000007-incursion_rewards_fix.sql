-- Additional incursion reward data for encounter types
-- +migrate Up

-- Encounter type rewards (client uses these IDs for reward chart display)
INSERT IGNORE INTO incursionRewards (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(5,  0, 5000000,   0, 500),    -- Scout / Entry-level
(6,  0, 7500000,   0, 800),    -- Light incursion
(7,  0, 8500000,   0, 1000),   -- Patrol
(8,  0, 10395000,  0, 1400),   -- Vanguard (encounter type 8)
(9,  0, 14000000,  0, 2500),   -- Assault (encounter type 9)
(10, 0, 25000000,  0, 5000),   -- Headquarters (encounter type 10)
(11, 0, 50000000,  0, 10000);  -- Mothership (encounter type 11)

-- +migrate Down
DELETE FROM incursionRewards WHERE rewardGroupID > 4;
