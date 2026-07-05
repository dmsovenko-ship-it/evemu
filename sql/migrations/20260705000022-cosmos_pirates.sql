-- Guristas COSMOS: E-8CSQ (Vale of the Silent nullsec)
-- +migrate Up

REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019737, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019738, 24, 1000166, 60015093, 1, 20, 12, 0),
(3019739, 24, 1000166, 60015093, 1, 20, 12, 0),
(3019740, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019741, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019742, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019743, 24, 1000166, 60015093, 1, 20, 12, 0),
(3019744, 24, 1000166, 60015093, 1, 20, 12, 0),
(3019745, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019746, 24, 1000166, 60015093, 1, 20, 12, 0),
(3019747, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019748, 24, 1000166, 60015093, 2, 20, 12, 0),
(3019749, 24, 1000166, 60015093, 6, 20, 12, 0),
(3019750, 24, 1000166, 60015093, 1, 20, 12, 0);

INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(92200, 0, 'Guristas C: Vale Recon', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(92201, 0, 'Guristas C: Iacta Space Data', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(92202, 0, 'Guristas C: Black Jack Intel', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(92203, 0, 'Guristas C: Contested Kois', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92200 AND 92203;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019737 AND 3019750;
