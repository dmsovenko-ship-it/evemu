-- Fix incursionRewards PK to allow multiple rows per rewardGroupID (separate ISK/LP entries)
-- +migrate Up

-- Drop and recreate with proper auto-increment PK
DROP TABLE IF EXISTS incursionRewards_v2;
CREATE TABLE incursionRewards_v2 (
    id INT(10) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    rewardGroupID INT(10) NOT NULL DEFAULT 0,
    rewardTypeID INT(10) NOT NULL DEFAULT 0,
    rewardQuantity INT(10) NOT NULL DEFAULT 0,
    lpTypeID INT(10) NOT NULL DEFAULT 0,
    lpAmount INT(10) NOT NULL DEFAULT 0,
    INDEX (rewardGroupID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Vanguard: ~10M ISK + 1400 CONCORD LP
-- rewardTypeID: 2 = ISK, 1 = LP (const.rewardTypeISK=2/rewardTypeLP=1, verified on live client)
-- LP rows store the LP amount in rewardQuantity so entries[].quantity carries it
INSERT INTO incursionRewards_v2 (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(1, 2, 10395000, 0, 0),
(1, 1, 1400, 29247, 1400);

-- Assault: ~18M ISK + 3500 LP
INSERT INTO incursionRewards_v2 (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(2, 2, 18200000, 0, 0),
(2, 1, 3500, 29247, 3500);

-- Headquarters: ~31M ISK + 7000 LP
INSERT INTO incursionRewards_v2 (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(3, 2, 31500000, 0, 0),
(3, 1, 7000, 29247, 7000);

-- Mothership: ~63M ISK + 14000 LP
INSERT INTO incursionRewards_v2 (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(4, 2, 63000000, 0, 0),
(4, 1, 14000, 29247, 14000);

-- Encounter-type rewards (used by GetDelayedRewardsByGroupIDs)
INSERT INTO incursionRewards_v2 (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(5,  2, 5000000,  0, 0), (5,  1, 500,  29247, 500),
(6,  2, 7500000,  0, 0), (6,  1, 800,  29247, 800),
(7,  2, 8500000,  0, 0), (7,  1, 1000, 29247, 1000),
(8,  2, 10395000, 0, 0), (8,  1, 1400, 29247, 1400),
(9,  2, 14000000, 0, 0), (9,  1, 2500, 29247, 2500),
(10, 2, 25000000, 0, 0), (10, 1, 5000, 29247, 5000),
(11, 2, 50000000, 0, 0), (11, 1, 10000, 29247, 10000);

-- Swap tables
RENAME TABLE incursionRewards TO incursionRewards_old;
RENAME TABLE incursionRewards_v2 TO incursionRewards;
DROP TABLE IF EXISTS incursionRewards_old;

-- +migrate Down
DROP TABLE IF EXISTS incursionRewards_v2;
CREATE TABLE incursionRewards (
    rewardGroupID INT(10) NOT NULL PRIMARY KEY,
    rewardTypeID INT(10) NOT NULL DEFAULT 0,
    rewardQuantity INT(10) NOT NULL DEFAULT 0,
    lpTypeID INT(10) NOT NULL DEFAULT 0,
    lpAmount INT(10) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
INSERT INTO incursionRewards (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(1, 0, 10395000, 0, 1400), (2, 0, 18200000, 0, 3500),
(3, 0, 31500000, 0, 7000), (4, 0, 63000000, 0, 14000);
