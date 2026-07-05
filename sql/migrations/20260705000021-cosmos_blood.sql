-- Blood Raider COSMOS: OK-FEM (Delve nullsec)
-- +migrate Up

REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019728, 24, 1000064, 60015093, 6, 20, 12, 0),
(3019729, 24, 1000064, 60015093, 5, 20, 12, 0),
(3019730, 24, 1000064, 60015093, 1, 20, 12, 0),
(3019731, 24, 1000064, 60015093, 1, 20, 12, 0),
(3019732, 24, 1000064, 60015093, 5, 20, 12, 0),
(3019733, 24, 1000064, 60015093, 6, 20, 12, 0),
(3019734, 24, 1000064, 60015093, 1, 20, 12, 0),
(3019735, 24, 1000064, 60015093, 6, 20, 12, 0),
(3019736, 24, 1000064, 60015093, 1, 20, 12, 0);

INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(92195, 0, 'Blood C: Covenant Intel', 6, 9, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(92196, 0, 'Blood C: Delve Recon', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92195 AND 92196;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019728 AND 3019736;
