-- Mining missions (Crucible-era) missing from qstMining
-- Excluded: AIR Program chains (Augumene, Passing the ISK),
--           Odyssey gas harvesting (Gas Injections, Like Drones to a Cloud)
-- +migrate Up

INSERT IGNORE INTO `qstMining` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
-- Level 1
(91000, 0, 'Asteroid Catastrophe', 1, 5, 1, 0, 0, 0, 0, 1, 10000, 0, 0, 2000, 0),
(91001, 0, 'Bountiful Banidine', 1, 5, 1, 0, 0, 0, 0, 1, 10000, 0, 0, 2000, 0),
(91002, 0, 'Burnt Traces', 1, 5, 1, 0, 0, 0, 0, 1, 10000, 0, 0, 2000, 0),
(91003, 0, 'Mercium Experiments', 1, 5, 1, 0, 0, 0, 0, 1, 10000, 0, 0, 2000, 0),
(91004, 0, 'Starting Simple', 1, 5, 1, 0, 0, 0, 0, 1, 10000, 0, 0, 2000, 0),
-- Level 2
(91005, 0, 'Claimjumpers', 2, 5, 3, 0, 0, 0, 0, 1, 25000, 0, 0, 5000, 0),
(91006, 0, 'Down and Dirty', 2, 5, 3, 0, 0, 0, 0, 1, 25000, 0, 0, 5000, 0),
(91007, 0, 'Mercium Belt', 2, 5, 3, 0, 0, 0, 0, 1, 25000, 0, 0, 5000, 0),
(91008, 0, 'Unknown Events', 2, 5, 3, 0, 0, 0, 0, 1, 25000, 0, 0, 5000, 0),
-- Level 3
(91009, 0, 'A Better World', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91010, 0, 'Beware They Live', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91011, 0, 'Coming ''Round the Mountain', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91012, 0, 'Drone Distribution', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91013, 0, 'Persistent Pests', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91014, 0, 'Pile of Pithix', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
(91015, 0, 'Stay Frosty', 3, 5, 5, 0, 0, 0, 0, 1, 60000, 0, 0, 12000, 0),
-- Level 4
(91016, 0, 'Arisite Envy', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91017, 0, 'Cheap Chills', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91018, 0, 'Feeding the Giant', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91019, 0, 'Geodite and Gemology', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91020, 0, 'Ice Installation', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91021, 0, 'Mother Lode', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0),
(91022, 0, 'Not Gneiss at All', 4, 5, 7, 0, 0, 0, 0, 1, 150000, 0, 0, 30000, 0);

-- +migrate Down
DELETE FROM `qstMining` WHERE `id` BETWEEN 91000 AND 91022;
