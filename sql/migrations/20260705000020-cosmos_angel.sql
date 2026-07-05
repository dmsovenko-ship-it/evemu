-- Angel COSMOS: I-3ODK (Feythabolis nullsec)
-- +migrate Up

REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019715, 24, 1000157, 60015093, 1, 20, 12, 0),
(3019716, 24, 1000157, 60015093, 5, 20, 12, 0),
(3019717, 24, 1000157, 60015093, 1, 20, 12, 0),
(3019718, 24, 1000157, 60015093, 1, 20, 12, 0),
(3019719, 24, 1000157, 60015093, 5, 20, 12, 0),
(3019720, 24, 1000157, 60015093, 1, 20, 12, 0),
(3019721, 24, 1000157, 60015093, 5, 20, 12, 0),
(3019722, 24, 1000157, 60015093, 7, 20, 12, 0),
(3019723, 24, 1000157, 60015093, 5, 20, 12, 0),
(3019724, 24, 1000157, 60015093, 7, 20, 12, 0),
(3019725, 24, 1000157, 60015093, 7, 20, 12, 0),
(3019726, 24, 1000157, 60015093, 5, 20, 12, 0),
(3019727, 24, 1000157, 60015093, 1, 20, 12, 0);

INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(92190, 0, 'Angel C: Flags of Blood', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92191, 0, 'Angel C: Little Sister', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92192, 0, 'Angel C: Minecore Rescue', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92193, 0, 'Angel C: The Wind Chimes', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92194, 0, 'Angel C: For Fear of Akrada', 5, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92190 AND 92194;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019715 AND 3019727;
