-- Storyline missions from EVE University wiki "Storyline mission reports"
-- Mission typeID=8 for all storyline missions
-- +migrate Up

-- New table for encounter-type missions (was missing from base SDE)
CREATE TABLE IF NOT EXISTS `qstEncounter` (
  `id` int(5) NOT NULL DEFAULT 0,
  `briefingID` int(5) NOT NULL DEFAULT 0,
  `name` text DEFAULT NULL,
  `level` tinyint(1) NOT NULL DEFAULT 0,
  `typeID` tinyint(1) NOT NULL DEFAULT 0,
  `sysRange` tinyint(2) NOT NULL DEFAULT 1,
  `important` bit(1) NOT NULL DEFAULT b'0',
  `storyline` bit(1) NOT NULL DEFAULT b'0',
  `raceID` tinyint(2) NOT NULL DEFAULT 0,
  `dungeonID` int(10) NOT NULL DEFAULT 0,
  `rewardISK` int(10) NOT NULL DEFAULT 0,
  `rewardItemID` int(11) NOT NULL DEFAULT 0,
  `rewardItemQty` int(11) NOT NULL DEFAULT 0,
  `bonusISK` int(11) NOT NULL DEFAULT 0,
  `bonusTime` int(10) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Courier Storyline Missions (typeID=8) into existing qstCourier table
-- Level 1
INSERT INTO `qstCourier` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`, `collateral`) VALUES
(60000, 0, 'A Cargo With Attitude', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60001, 0, 'A Different Drone', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60002, 0, 'A Greener World', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60003, 0, 'A Little Work On The Side', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60004, 0, 'A Piece of History', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60005, 0, 'A Special Delivery', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60006, 0, 'A Watchful Eye', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60007, 0, 'Ancient Treasures', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60008, 0, 'Ditanium', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60009, 0, 'Divine Intervention', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60010, 0, 'Evacuation', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60011, 0, 'Fire and Ice', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60012, 0, 'Heart of the Rogue Drone', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60013, 0, 'Of Fangs and Claws', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60014, 0, 'On the Run', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60015, 0, 'Opiate of the Masses', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60016, 0, 'Operation Doorstop', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60017, 0, 'Pieces of the Past', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60018, 0, 'The Creeping Cold', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60019, 0, 'The Essence of Speed', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60020, 0, 'Wartime Advances', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60021, 0, 'Culture Clash', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60022, 0, 'A Father''s Love', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60023, 0, 'Very Important Pirates', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60024, 0, 'Transaction Data Delivery', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000);

-- Level 2 Courier Storyline
INSERT INTO `qstCourier` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`, `collateral`) VALUES
(60025, 0, 'A Load of Scrap', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60026, 0, 'Brand New Harvesters', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60027, 0, 'Shifting Rocks', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60028, 0, 'Fire and Ice', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60029, 0, 'Heart of the Rogue Drone', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60030, 0, 'Opiate of the Masses', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60031, 0, 'The Creeping Cold', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60032, 0, 'Amphibian Error', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60033, 0, 'Black Ops Crisis', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60034, 0, 'Send the Marines!', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60035, 0, 'Transaction Data Delivery', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60036, 0, 'Unmasking the Traitor', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60037, 0, 'A Desperate Rescue', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60038, 0, 'A Special Delivery', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60039, 0, 'The State of the Empire', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000),
(60040, 0, 'Their Secret Defense', 2, 8, 1, b'0', b'1', 0, 0, 1, 25000, 0, 0, 5000, 0, 50000);

-- Level 3 Courier Storyline
INSERT INTO `qstCourier` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`, `collateral`) VALUES
(60041, 0, 'A Different Drone', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60042, 0, 'A Greener World', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60043, 0, 'A Little Work On The Side', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60044, 0, 'Ancient Treasures', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60045, 0, 'Wartime Advances', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60046, 0, 'Culture Clash', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60047, 0, 'Eradication', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60048, 0, 'A Father''s Love', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000),
(60049, 0, 'A Special Delivery', 3, 8, 1, b'0', b'1', 0, 0, 1, 70000, 0, 0, 14000, 0, 140000);

-- Level 4 Courier Storyline
INSERT INTO `qstCourier` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`, `collateral`) VALUES
(60050, 0, 'Fire and Ice', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60051, 0, 'Heart of the Rogue Drone', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60052, 0, 'Opiate of the Masses', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60053, 0, 'The Creeping Cold', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60054, 0, 'Amphibian Error', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60055, 0, 'Black Ops Crisis', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60056, 0, 'Send the Marines!', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60057, 0, 'Shifting Rocks', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60058, 0, 'Transaction Data Delivery', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60059, 0, 'The Governor''s Ball', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60060, 0, 'The State of the Empire', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000),
(60061, 0, 'A Special Delivery', 4, 8, 1, b'0', b'1', 0, 0, 1, 175000, 0, 0, 35000, 0, 350000);

-- Chain Courier Missions (courier parts of multi-part arcs)
INSERT INTO `qstCourier` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `itemTypeID`, `itemQty`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`, `collateral`) VALUES
(60062, 0, 'Another Slave Rescue (Part 3/3)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60063, 0, 'Guristas Strike (Part 8/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60064, 0, 'Guristas Strike (Part 9/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60065, 0, 'Guristas Strike (Part 10/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60066, 0, 'Kidnappers Strike (Part 8/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60067, 0, 'Kidnappers Strike (Part 9/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60068, 0, 'Kidnappers Strike (Part 10/10)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60069, 0, 'The Gallente Archaeologist (Part 6/9)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60070, 0, 'The Gallente Archaeologist (Part 7/9)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60071, 0, 'The Gallente Archaeologist (Part 8/9)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60072, 0, 'The Gallente Archaeologist (Part 9/9)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000),
(60073, 0, 'The Numon Claim (Part 5/5)', 1, 8, 1, b'0', b'1', 0, 0, 1, 10000, 0, 0, 2000, 0, 20000);

-- Encounter Storyline Missions (typeID=8) into new qstEncounter table
-- Level 1
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60074, 0, 'A Case of Kidnapping', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60075, 0, 'An End to Eavesdropping', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60076, 0, 'Amarrian Tyrants', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60077, 0, 'Search and Rescue', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60078, 0, 'The Heir''s Favorite Slave', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0);

-- Level 2 Encounter
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60079, 0, 'Ambushed Ally', 2, 8, 1, b'0', b'1', 0, 0, 25000, 0, 0, 5000, 0),
(60080, 0, 'Clearing a Path', 2, 8, 1, b'0', b'1', 0, 0, 25000, 0, 0, 5000, 0),
(60081, 0, 'Fetching Friends', 2, 8, 1, b'0', b'1', 0, 0, 25000, 0, 0, 5000, 0);

-- Level 3 Encounter
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60082, 0, 'A Force to be Reckoned with (Angel Cartel)', 3, 8, 1, b'0', b'1', 0, 0, 70000, 0, 0, 14000, 0),
(60083, 0, 'A Force to be Reckoned with (Blood Raiders)', 3, 8, 1, b'0', b'1', 0, 0, 70000, 0, 0, 14000, 0),
(60084, 0, 'A Force to be Reckoned with (Serpentis)', 3, 8, 1, b'0', b'1', 0, 0, 70000, 0, 0, 14000, 0),
(60085, 0, 'Id and Egonics, Inc.', 3, 8, 1, b'0', b'1', 0, 0, 70000, 0, 0, 14000, 0),
(60086, 0, 'Pirate Radio', 3, 8, 1, b'0', b'1', 0, 0, 70000, 0, 0, 14000, 0);

-- Level 4 Encounter
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60087, 0, 'Blood Farm', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60088, 0, 'Crowd Control', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60089, 0, 'Diplomatic Incident', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60090, 0, 'Dissidents', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60091, 0, 'Evolution', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60092, 0, 'Extract The Renegade', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60093, 0, 'Forgotten Outpost', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60094, 0, 'Gate to Nowhere', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60095, 0, 'Hidden Hope', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60096, 0, 'Illegal Mining', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60097, 0, 'Innocents In The Crossfire', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60098, 0, 'Insorum Hijacking', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60099, 0, 'Jealous Rivals', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60100, 0, 'Lamb Amongst Lions', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60101, 0, 'Matriarch', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60102, 0, 'Nine Tenths Of The Wormhole', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60103, 0, 'Patient Zero', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60104, 0, 'Persistent Enemy', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60105, 0, 'Prison Transfer', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60106, 0, 'Quota Season', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60107, 0, 'Racetrack Ruckus', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60108, 0, 'Record Cleaning', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60109, 0, 'Serpentis Ship Builders', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60110, 0, 'Soothe The Salvage Beast', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60111, 0, 'Stem The Flow', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60112, 0, 'The Blood of Angry Men', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60113, 0, 'The Mouthy Merc', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60114, 0, 'Warlord Strikes', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0);

-- Multi-part Chain Encounter Missions
-- Level 1 chain parts
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60115, 0, 'Another Slave Rescue (Part 1/3)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60116, 0, 'Another Slave Rescue (Part 2/3)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60117, 0, 'Guristas Strike (Part 1/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60118, 0, 'Guristas Strike (Part 2/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60119, 0, 'Guristas Strike (Part 3/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60120, 0, 'Guristas Strike (Part 4/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60121, 0, 'Guristas Strike (Part 5/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60122, 0, 'Guristas Strike (Part 6/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60123, 0, 'Guristas Strike (Part 7/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60124, 0, 'Kidnappers Strike (Part 1/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60125, 0, 'Kidnappers Strike (Part 2/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60126, 0, 'Kidnappers Strike (Part 3/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60127, 0, 'Kidnappers Strike (Part 4/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60128, 0, 'Kidnappers Strike (Part 5/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60129, 0, 'Kidnappers Strike (Part 6/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60130, 0, 'Kidnappers Strike (Part 7/10)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60131, 0, 'The Gallente Archaeologist (Part 1/9)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60132, 0, 'The Gallente Archaeologist (Part 2/9)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60133, 0, 'The Gallente Archaeologist (Part 3/9)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60134, 0, 'The Gallente Archaeologist (Part 4/9)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60135, 0, 'The Gallente Archaeologist (Part 5/9)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60136, 0, 'The Numon Claim (Part 1/5)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60137, 0, 'The Numon Claim (Part 2/5)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60138, 0, 'The Numon Claim (Part 3/5)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0),
(60139, 0, 'The Numon Claim (Part 4/5)', 1, 8, 1, b'0', b'1', 0, 0, 10000, 0, 0, 2000, 0);

-- Level 4 chain encounter (Shipyard Theft faction variants)
INSERT INTO `qstEncounter` (`id`, `briefingID`, `name`, `level`, `typeID`, `sysRange`, `important`, `storyline`, `raceID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(60140, 0, 'Shipyard Theft (Angel Cartel)', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60141, 0, 'Shipyard Theft (Blood Raiders)', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60142, 0, 'Shipyard Theft (Guristas Pirates)', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0),
(60143, 0, 'Shipyard Theft (Serpentis)', 4, 8, 1, b'0', b'1', 0, 0, 175000, 0, 0, 35000, 0);

-- +migrate Down
DROP TABLE IF EXISTS `qstEncounter`;
DELETE FROM `qstCourier` WHERE `typeID` = 8 AND `id` BETWEEN 60000 AND 60143;
