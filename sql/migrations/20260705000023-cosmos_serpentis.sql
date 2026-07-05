-- Serpentis COSMOS: Pegasus (Fountain nullsec, 14 agents)
-- +migrate Up

REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019751, 24, 1000165, 60015093, 6, 20, 12, 0),
(3019752, 24, 1000165, 60015093, 5, 20, 12, 0),
(3019753, 24, 1000165, 60015093, 5, 20, 12, 0),
(3019754, 24, 1000165, 60015093, 1, 20, 12, 0),
(3019755, 24, 1000165, 60015093, 1, 20, 12, 0),
(3019756, 24, 1000165, 60015093, 9, 20, 12, 0),
(3019757, 24, 1000165, 60015093, 8, 20, 12, 0),
(3019758, 24, 1000165, 60015093, 5, 20, 12, 0),
(3019759, 24, 1000165, 60015093, 1, 20, 12, 0),
(3019760, 24, 1000165, 60015093, 5, 20, 12, 0),
(3019761, 24, 1000165, 60015093, 9, 20, 12, 0),
(3019762, 24, 1000165, 60015093, 1, 20, 12, 0),
(3019763, 24, 1000165, 60015093, 1, 20, 12, 0),
(3019764, 24, 1000165, 60015093, 5, 20, 12, 0);

INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(92204, 0, 'Serp C: Fountain Survey', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(92205, 0, 'Serp C: Yakura Intel', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92206, 0, 'Serp C: Tag Turn-In (Silver)', 8, 9, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(92207, 0, 'Serp C: Tag Turn-In (Gold)', 9, 9, 0, 0, 0, 0, 0, 0, 750000, 0, 0, 150000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92204 AND 92207;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019751 AND 3019764;
