-- Fix missing NPC typeIDs for anomaly dungeons and mission encounters
-- Old typeIDs (5832-5949) either conflict with SDE modules or are missing.
-- We relocate them to the free 33000+ range.
-- +migrate Up

-- Widen typeID for values > 32767 (COSMOS already uses 99000+)
ALTER TABLE `invTypes` MODIFY `typeID` int(10) NOT NULL DEFAULT '0';

-- ====== Guristas (faction 500014, raceID=1) ======
-- Groups: Frigate=562, Cruiser=561, Battleship=560, Destroyer=579, BattleCruiser=580
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33001, 562, 'Guristas Scout', 'Guristas frigate.', 1500100, 15001, 45, 1, 1, 0, 0),
(33002, 561, 'Guristas Raider', 'Guristas cruiser.', 2500000, 25000, 100, 1, 1, 0, 0),
(33003, 560, 'Guristas Warlord', 'Guristas battleship.', 5000000, 50000, 200, 1, 1, 0, 0),
(33004, 580, 'Guristas Baron', 'Guristas battlecruiser.', 3500000, 35000, 150, 1, 1, 0, 0),
(33005, 562, 'Guristas Hound', 'Guristas frigate. Fast attack variant.', 1600100, 16001, 50, 1, 1, 0, 0),
(33006, 561, 'Guristas Marauder', 'Guristas cruiser. Heavy variant.', 2700000, 27000, 110, 1, 1, 0, 0),
(33007, 561, 'Guristas Predator', 'Guristas cruiser. Command variant.', 2800000, 28000, 115, 1, 1, 0, 0),
(33008, 579, 'Guristas Thrasher', 'Guristas destroyer.', 2000000, 20000, 80, 1, 1, 0, 0),
(33009, 561, 'Guristas Commodore', 'Guristas cruiser. Advanced variant.', 2900000, 29000, 120, 1, 1, 0, 0),
(33010, 560, 'Guristas Warlord II', 'Guristas battleship. Heavy variant.', 5200000, 52000, 210, 1, 1, 0, 0),
(33011, 560, 'Guristas Overlord', 'Guristas battleship. Command variant.', 5500000, 55000, 220, 1, 1, 0, 0),
(33012, 560, 'Guristas Dreadnought', 'Guristas battleship. Elite vessel.', 5800000, 58000, 230, 1, 1, 0, 0),
(33013, 560, 'Guristas Leviathan', 'Guristas battleship. Heavy command variant.', 6000000, 60000, 240, 1, 1, 0, 0),
(33014, 580, 'Guristas Kingpin', 'Guristas battlecruiser. Commander vessel.', 3800000, 38000, 160, 1, 1, 0, 0);

-- ====== Angel Cartel (faction 500011, raceID=2) ======
-- Groups: Frigate=550, Cruiser=551, Battleship=552, Destroyer=575, BattleCruiser=576
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33020, 550, 'Angel Ruffian', 'Angel frigate.', 2250000, 22500, 75, 1, 2, 0, 0),
(33021, 551, 'Angel Marauder', 'Angel cruiser.', 3000000, 30000, 120, 1, 2, 0, 0),
(33022, 552, 'Angel Warlord', 'Angel battleship.', 6000000, 60000, 250, 1, 2, 0, 0),
(33023, 576, 'Angel Baron', 'Angel battlecruiser.', 4000000, 40000, 180, 1, 2, 0, 0),
(33024, 550, 'Angel Outlaw', 'Angel frigate. Fast attack variant.', 2350000, 23500, 80, 1, 2, 0, 0),
(33025, 551, 'Angel Ravager', 'Angel cruiser. Heavy variant.', 3200000, 32000, 130, 1, 2, 0, 0),
(33026, 551, 'Angel Reaver', 'Angel cruiser. Command variant.', 3300000, 33000, 135, 1, 2, 0, 0),
(33027, 551, 'Angel Commodore', 'Angel cruiser. Advanced variant.', 3400000, 34000, 140, 1, 2, 0, 0),
(33028, 552, 'Angel Warlord II', 'Angel battleship. Heavy variant.', 6200000, 62000, 260, 1, 2, 0, 0),
(33029, 552, 'Angel Overlord', 'Angel battleship. Command variant.', 6500000, 65000, 270, 1, 2, 0, 0),
(33030, 552, 'Angel Dreadnought', 'Angel battleship. Elite vessel.', 6800000, 68000, 280, 1, 2, 0, 0),
(33031, 552, 'Angel Leviathan', 'Angel battleship. Heavy command variant.', 7000000, 70000, 290, 1, 2, 0, 0),
(33032, 576, 'Angel Kingpin', 'Angel battlecruiser. Commander vessel.', 4200000, 42000, 190, 1, 2, 0, 0);

-- ====== Serpentis (faction 500013, raceID=8) ======
-- Groups: Frigate=572, Cruiser=571, Battleship=570, Destroyer=583, BattleCruiser=584
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33040, 572, 'Serpentis Scout', 'Serpentis frigate.', 2450000, 24500, 60, 1, 8, 0, 0),
(33041, 571, 'Serpentis Raider', 'Serpentis cruiser.', 3200000, 32000, 130, 1, 8, 0, 0),
(33042, 570, 'Serpentis Warlord', 'Serpentis battleship.', 5500000, 55000, 220, 1, 8, 0, 0),
(33043, 584, 'Serpentis Baron', 'Serpentis battlecruiser.', 3800000, 38000, 160, 1, 8, 0, 0),
(33044, 572, 'Serpentis Hound', 'Serpentis frigate. Fast attack variant.', 2550000, 25500, 65, 1, 8, 0, 0),
(33045, 571, 'Serpentis Marauder', 'Serpentis cruiser. Heavy variant.', 3400000, 34000, 140, 1, 8, 0, 0),
(33046, 571, 'Serpentis Predator', 'Serpentis cruiser. Command variant.', 3500000, 35000, 145, 1, 8, 0, 0),
(33047, 571, 'Serpentis Commodore', 'Serpentis cruiser. Advanced variant.', 3600000, 36000, 150, 1, 8, 0, 0),
(33048, 570, 'Serpentis Warlord II', 'Serpentis battleship. Heavy variant.', 5700000, 57000, 230, 1, 8, 0, 0),
(33049, 570, 'Serpentis Overlord', 'Serpentis battleship. Command variant.', 6000000, 60000, 240, 1, 8, 0, 0),
(33050, 570, 'Serpentis Dreadnought', 'Serpentis battleship. Elite vessel.', 6300000, 63000, 250, 1, 8, 0, 0),
(33051, 570, 'Serpentis Leviathan', 'Serpentis battleship. Heavy command variant.', 6500000, 65000, 260, 1, 8, 0, 0),
(33052, 584, 'Serpentis Kingpin', 'Serpentis battlecruiser. Commander vessel.', 4000000, 40000, 170, 1, 8, 0, 0);

-- ====== Blood Raiders (faction 500012, raceID=4) ======
-- Groups: Frigate=557, Cruiser=555, Battleship=556, Destroyer=577, BattleCruiser=578
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33060, 557, 'Blood Scout', 'Blood frigate.', 2000000, 20000, 55, 1, 4, 0, 0),
(33061, 555, 'Blood Raider', 'Blood cruiser.', 2800000, 28000, 110, 1, 4, 0, 0),
(33062, 556, 'Blood Warlord', 'Blood battleship.', 5200000, 52000, 210, 1, 4, 0, 0),
(33063, 578, 'Blood Baron', 'Blood battlecruiser.', 3600000, 36000, 155, 1, 4, 0, 0),
(33064, 557, 'Blood Hound', 'Blood frigate. Fast attack variant.', 2100000, 21000, 60, 1, 4, 0, 0),
(33065, 555, 'Blood Marauder', 'Blood cruiser. Heavy variant.', 3000000, 30000, 120, 1, 4, 0, 0),
(33066, 555, 'Blood Predator', 'Blood cruiser. Command variant.', 3100000, 31000, 125, 1, 4, 0, 0),
(33067, 555, 'Blood Commodore', 'Blood cruiser. Advanced variant.', 3200000, 32000, 130, 1, 4, 0, 0),
(33068, 556, 'Blood Warlord II', 'Blood battleship. Heavy variant.', 5400000, 54000, 220, 1, 4, 0, 0),
(33069, 556, 'Blood Overlord', 'Blood battleship. Command variant.', 5700000, 57000, 230, 1, 4, 0, 0),
(33070, 556, 'Blood Dreadnought', 'Blood battleship. Elite vessel.', 6000000, 60000, 240, 1, 4, 0, 0),
(33071, 556, 'Blood Leviathan', 'Blood battleship. Heavy command variant.', 6200000, 62000, 250, 1, 4, 0, 0),
(33072, 578, 'Blood Kingpin', 'Blood battlecruiser. Commander vessel.', 3800000, 38000, 165, 1, 4, 0, 0);

-- ====== Sansha's Nation (faction 500019, raceID=16) ======
-- Groups: Frigate=567, Cruiser=566, Battleship=565, Destroyer=581, BattleCruiser=582
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33080, 567, 'Sansha Scout', 'Sansha frigate.', 1900000, 19000, 50, 1, 16, 0, 0),
(33081, 566, 'Sansha Raider', 'Sansha cruiser.', 2700000, 27000, 105, 1, 16, 0, 0),
(33082, 565, 'Sansha Warlord', 'Sansha battleship.', 5100000, 51000, 200, 1, 16, 0, 0),
(33083, 582, 'Sansha Baron', 'Sansha battlecruiser.', 3500000, 35000, 150, 1, 16, 0, 0),
(33084, 567, 'Sansha Hound', 'Sansha frigate. Fast attack variant.', 2000000, 20000, 55, 1, 16, 0, 0),
(33085, 566, 'Sansha Marauder', 'Sansha cruiser. Heavy variant.', 2900000, 29000, 115, 1, 16, 0, 0),
(33086, 566, 'Sansha Commodore', 'Sansha cruiser. Advanced variant.', 3100000, 31000, 125, 1, 16, 0, 0),
(33087, 565, 'Sansha Warlord II', 'Sansha battleship. Heavy variant.', 5300000, 53000, 210, 1, 16, 0, 0),
(33088, 565, 'Sansha Dreadnought', 'Sansha battleship. Elite vessel.', 5600000, 56000, 220, 1, 16, 0, 0),
(33089, 565, 'Sansha Leviathan', 'Sansha battleship. Heavy command variant.', 5800000, 58000, 230, 1, 16, 0, 0),
(33090, 582, 'Sansha Kingpin', 'Sansha battlecruiser. Commander vessel.', 3700000, 37000, 160, 1, 16, 0, 0);

-- ====== Rogue Drones (faction 500020, raceID=0) ======
-- Groups: Frigate=759, Cruiser=757, Battleship=756, Destroyer=758, BattleCruiser=755
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33100, 759, 'Rogue Drone Scout', 'Rogue drone frigate.', 1800000, 18000, 40, 1, 0, 0, 0),
(33101, 757, 'Rogue Drone Raider', 'Rogue drone cruiser.', 2600000, 26000, 90, 1, 0, 0, 0),
(33102, 756, 'Rogue Drone Warlord', 'Rogue drone battleship.', 4900000, 49000, 180, 1, 0, 0, 0),
(33103, 755, 'Rogue Drone Baron', 'Rogue drone battlecruiser.', 3400000, 34000, 140, 1, 0, 0, 0),
(33104, 759, 'Rogue Drone Hound', 'Rogue drone frigate. Fast attack variant.', 1900000, 19000, 45, 1, 0, 0, 0),
(33105, 757, 'Rogue Drone Marauder', 'Rogue drone cruiser. Heavy variant.', 2800000, 28000, 100, 1, 0, 0, 0),
(33106, 757, 'Rogue Drone Commodore', 'Rogue drone cruiser. Advanced variant.', 3000000, 30000, 110, 1, 0, 0, 0),
(33107, 756, 'Rogue Drone Overlord', 'Rogue drone battleship. Command variant.', 5200000, 52000, 200, 1, 0, 0, 0),
(33108, 755, 'Rogue Drone Kingpin', 'Rogue drone battlecruiser. Commander vessel.', 3600000, 36000, 150, 1, 0, 0, 0);

-- ====== Mordu's Legion (faction 500010, raceID=8) ======
-- Groups: Frigate=699, Cruiser=701, Battleship=703, Destroyer=700, BattleCruiser=702
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33120, 699, 'Mordu Scout', 'Mordu frigate.', 2000000, 20000, 60, 1, 8, 0, 0),
(33121, 701, 'Mordu Raider', 'Mordu cruiser.', 3000000, 30000, 120, 1, 8, 0, 0),
(33122, 703, 'Mordu Warlord', 'Mordu battleship.', 5500000, 55000, 220, 1, 8, 0, 0),
(33123, 702, 'Mordu Baron', 'Mordu battlecruiser.', 3800000, 38000, 170, 1, 8, 0, 0),
(33124, 702, 'Mordu Commodore', 'Mordu battlecruiser. Advanced variant.', 4000000, 40000, 180, 1, 8, 0, 0),
(33125, 701, 'Mordu Marauder', 'Mordu cruiser. Heavy variant.', 3200000, 32000, 130, 1, 8, 0, 0),
(33126, 703, 'Mordu Warlord II', 'Mordu battleship. Heavy variant.', 5700000, 57000, 230, 1, 8, 0, 0),
(33127, 703, 'Mordu Overlord', 'Mordu battleship. Command variant.', 6000000, 60000, 240, 1, 8, 0, 0),
(33128, 703, 'Mordu Kingpin', 'Mordu battleship. Commander vessel.', 6300000, 63000, 250, 1, 8, 0, 0);

-- ====== DED / CONCORD (faction 500021, raceID=0) ======
-- Groups: Frigate=693, Cruiser=695, Battleship=697, Destroyer=694, BattleCruiser=696
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `published`) VALUES
(33140, 693, 'DED Scout', 'DED frigate.', 2200000, 22000, 65, 1, 0, 0, 0),
(33141, 695, 'DED Raider', 'DED cruiser.', 3100000, 31000, 125, 1, 0, 0, 0),
(33142, 697, 'DED Warlord', 'DED battleship.', 5600000, 56000, 230, 1, 0, 0, 0),
(33143, 696, 'DED Baron', 'DED battlecruiser.', 3900000, 39000, 175, 1, 0, 0, 0),
(33144, 696, 'DED Commodore', 'DED battlecruiser. Advanced variant.', 4100000, 41000, 185, 1, 0, 0, 0),
(33145, 695, 'DED Marauder', 'DED cruiser. Heavy variant.', 3300000, 33000, 135, 1, 0, 0, 0),
(33146, 697, 'DED Warlord II', 'DED battleship. Heavy variant.', 5800000, 58000, 240, 1, 0, 0, 0),
(33147, 697, 'DED Overlord', 'DED battleship. Command variant.', 6100000, 61000, 250, 1, 0, 0, 0),
(33148, 697, 'DED Kingpin', 'DED battleship. Commander vessel.', 6400000, 64000, 260, 1, 0, 0, 0);

-- Relink dunRoomObjects: old typeIDs → new typeIDs
UPDATE `dunRoomObjects` SET `typeID` = 33001 WHERE `typeID` = 5832;
UPDATE `dunRoomObjects` SET `typeID` = 33002 WHERE `typeID` = 5833;
UPDATE `dunRoomObjects` SET `typeID` = 33003 WHERE `typeID` = 5834;
UPDATE `dunRoomObjects` SET `typeID` = 33004 WHERE `typeID` = 5835;
UPDATE `dunRoomObjects` SET `typeID` = 33005 WHERE `typeID` = 5836;
UPDATE `dunRoomObjects` SET `typeID` = 33006 WHERE `typeID` = 5837;
UPDATE `dunRoomObjects` SET `typeID` = 33007 WHERE `typeID` = 5838;
UPDATE `dunRoomObjects` SET `typeID` = 33008 WHERE `typeID` = 5839;
UPDATE `dunRoomObjects` SET `typeID` = 33009 WHERE `typeID` = 5840;
UPDATE `dunRoomObjects` SET `typeID` = 33010 WHERE `typeID` = 5841;
UPDATE `dunRoomObjects` SET `typeID` = 33011 WHERE `typeID` = 5842;
UPDATE `dunRoomObjects` SET `typeID` = 33012 WHERE `typeID` = 5843;
UPDATE `dunRoomObjects` SET `typeID` = 33013 WHERE `typeID` = 5844;
UPDATE `dunRoomObjects` SET `typeID` = 33014 WHERE `typeID` = 5845;
UPDATE `dunRoomObjects` SET `typeID` = 33020 WHERE `typeID` = 5861;
UPDATE `dunRoomObjects` SET `typeID` = 33021 WHERE `typeID` = 5862;
UPDATE `dunRoomObjects` SET `typeID` = 33022 WHERE `typeID` = 5863;
UPDATE `dunRoomObjects` SET `typeID` = 33023 WHERE `typeID` = 5864;
UPDATE `dunRoomObjects` SET `typeID` = 33024 WHERE `typeID` = 5865;
UPDATE `dunRoomObjects` SET `typeID` = 33025 WHERE `typeID` = 5866;
UPDATE `dunRoomObjects` SET `typeID` = 33026 WHERE `typeID` = 5867;
UPDATE `dunRoomObjects` SET `typeID` = 33027 WHERE `typeID` = 5868;
UPDATE `dunRoomObjects` SET `typeID` = 33028 WHERE `typeID` = 5869;
UPDATE `dunRoomObjects` SET `typeID` = 33029 WHERE `typeID` = 5870;
UPDATE `dunRoomObjects` SET `typeID` = 33030 WHERE `typeID` = 5871;
UPDATE `dunRoomObjects` SET `typeID` = 33031 WHERE `typeID` = 5872;
UPDATE `dunRoomObjects` SET `typeID` = 33032 WHERE `typeID` = 5873;
UPDATE `dunRoomObjects` SET `typeID` = 33040 WHERE `typeID` = 5878;
UPDATE `dunRoomObjects` SET `typeID` = 33041 WHERE `typeID` = 5879;
UPDATE `dunRoomObjects` SET `typeID` = 33042 WHERE `typeID` = 5880;
UPDATE `dunRoomObjects` SET `typeID` = 33043 WHERE `typeID` = 5881;
UPDATE `dunRoomObjects` SET `typeID` = 33044 WHERE `typeID` = 5882;
UPDATE `dunRoomObjects` SET `typeID` = 33045 WHERE `typeID` = 5883;
UPDATE `dunRoomObjects` SET `typeID` = 33046 WHERE `typeID` = 5884;
UPDATE `dunRoomObjects` SET `typeID` = 33047 WHERE `typeID` = 5885;
UPDATE `dunRoomObjects` SET `typeID` = 33048 WHERE `typeID` = 5886;
UPDATE `dunRoomObjects` SET `typeID` = 33049 WHERE `typeID` = 5887;
UPDATE `dunRoomObjects` SET `typeID` = 33050 WHERE `typeID` = 5888;
UPDATE `dunRoomObjects` SET `typeID` = 33051 WHERE `typeID` = 5889;
UPDATE `dunRoomObjects` SET `typeID` = 33052 WHERE `typeID` = 5890;
UPDATE `dunRoomObjects` SET `typeID` = 33060 WHERE `typeID` = 5891;
UPDATE `dunRoomObjects` SET `typeID` = 33061 WHERE `typeID` = 5892;
UPDATE `dunRoomObjects` SET `typeID` = 33062 WHERE `typeID` = 5893;
UPDATE `dunRoomObjects` SET `typeID` = 33063 WHERE `typeID` = 5894;
UPDATE `dunRoomObjects` SET `typeID` = 33064 WHERE `typeID` = 5895;
UPDATE `dunRoomObjects` SET `typeID` = 33065 WHERE `typeID` = 5896;
UPDATE `dunRoomObjects` SET `typeID` = 33066 WHERE `typeID` = 5897;
UPDATE `dunRoomObjects` SET `typeID` = 33067 WHERE `typeID` = 5898;
UPDATE `dunRoomObjects` SET `typeID` = 33068 WHERE `typeID` = 5899;
UPDATE `dunRoomObjects` SET `typeID` = 33069 WHERE `typeID` = 5900;
UPDATE `dunRoomObjects` SET `typeID` = 33070 WHERE `typeID` = 5901;
UPDATE `dunRoomObjects` SET `typeID` = 33071 WHERE `typeID` = 5902;
UPDATE `dunRoomObjects` SET `typeID` = 33072 WHERE `typeID` = 5903;
UPDATE `dunRoomObjects` SET `typeID` = 33080 WHERE `typeID` = 5905;
UPDATE `dunRoomObjects` SET `typeID` = 33081 WHERE `typeID` = 5906;
UPDATE `dunRoomObjects` SET `typeID` = 33082 WHERE `typeID` = 5907;
UPDATE `dunRoomObjects` SET `typeID` = 33083 WHERE `typeID` = 5908;
UPDATE `dunRoomObjects` SET `typeID` = 33084 WHERE `typeID` = 5909;
UPDATE `dunRoomObjects` SET `typeID` = 33085 WHERE `typeID` = 5910;
UPDATE `dunRoomObjects` SET `typeID` = 33086 WHERE `typeID` = 5915;
UPDATE `dunRoomObjects` SET `typeID` = 33087 WHERE `typeID` = 5916;
UPDATE `dunRoomObjects` SET `typeID` = 33088 WHERE `typeID` = 5917;
UPDATE `dunRoomObjects` SET `typeID` = 33089 WHERE `typeID` = 5918;
UPDATE `dunRoomObjects` SET `typeID` = 33090 WHERE `typeID` = 5919;
UPDATE `dunRoomObjects` SET `typeID` = 33100 WHERE `typeID` = 5911;
UPDATE `dunRoomObjects` SET `typeID` = 33101 WHERE `typeID` = 5912;
UPDATE `dunRoomObjects` SET `typeID` = 33102 WHERE `typeID` = 5913;
UPDATE `dunRoomObjects` SET `typeID` = 33103 WHERE `typeID` = 5914;
UPDATE `dunRoomObjects` SET `typeID` = 33104 WHERE `typeID` = 5921;
UPDATE `dunRoomObjects` SET `typeID` = 33105 WHERE `typeID` = 5922;
UPDATE `dunRoomObjects` SET `typeID` = 33106 WHERE `typeID` = 5923;
UPDATE `dunRoomObjects` SET `typeID` = 33107 WHERE `typeID` = 5924;
UPDATE `dunRoomObjects` SET `typeID` = 33108 WHERE `typeID` = 5925;
UPDATE `dunRoomObjects` SET `typeID` = 33120 WHERE `typeID` = 5931;
UPDATE `dunRoomObjects` SET `typeID` = 33121 WHERE `typeID` = 5932;
UPDATE `dunRoomObjects` SET `typeID` = 33122 WHERE `typeID` = 5933;
UPDATE `dunRoomObjects` SET `typeID` = 33123 WHERE `typeID` = 5934;
UPDATE `dunRoomObjects` SET `typeID` = 33124 WHERE `typeID` = 5935;
UPDATE `dunRoomObjects` SET `typeID` = 33125 WHERE `typeID` = 5936;
UPDATE `dunRoomObjects` SET `typeID` = 33126 WHERE `typeID` = 5937;
UPDATE `dunRoomObjects` SET `typeID` = 33127 WHERE `typeID` = 5938;
UPDATE `dunRoomObjects` SET `typeID` = 33128 WHERE `typeID` = 5939;
UPDATE `dunRoomObjects` SET `typeID` = 33140 WHERE `typeID` = 5941;
UPDATE `dunRoomObjects` SET `typeID` = 33141 WHERE `typeID` = 5942;
UPDATE `dunRoomObjects` SET `typeID` = 33142 WHERE `typeID` = 5943;
UPDATE `dunRoomObjects` SET `typeID` = 33143 WHERE `typeID` = 5944;
UPDATE `dunRoomObjects` SET `typeID` = 33144 WHERE `typeID` = 5945;
UPDATE `dunRoomObjects` SET `typeID` = 33145 WHERE `typeID` = 5946;
UPDATE `dunRoomObjects` SET `typeID` = 33146 WHERE `typeID` = 5947;
UPDATE `dunRoomObjects` SET `typeID` = 33147 WHERE `typeID` = 5948;
UPDATE `dunRoomObjects` SET `typeID` = 33148 WHERE `typeID` = 5949;

-- +migrate Down
UPDATE `dunRoomObjects` SET `typeID` = 5832 WHERE `typeID` = 33001;
UPDATE `dunRoomObjects` SET `typeID` = 5833 WHERE `typeID` = 33002;
UPDATE `dunRoomObjects` SET `typeID` = 5834 WHERE `typeID` = 33003;
UPDATE `dunRoomObjects` SET `typeID` = 5835 WHERE `typeID` = 33004;
UPDATE `dunRoomObjects` SET `typeID` = 5836 WHERE `typeID` = 33005;
UPDATE `dunRoomObjects` SET `typeID` = 5837 WHERE `typeID` = 33006;
UPDATE `dunRoomObjects` SET `typeID` = 5838 WHERE `typeID` = 33007;
UPDATE `dunRoomObjects` SET `typeID` = 5839 WHERE `typeID` = 33008;
UPDATE `dunRoomObjects` SET `typeID` = 5840 WHERE `typeID` = 33009;
UPDATE `dunRoomObjects` SET `typeID` = 5841 WHERE `typeID` = 33010;
UPDATE `dunRoomObjects` SET `typeID` = 5842 WHERE `typeID` = 33011;
UPDATE `dunRoomObjects` SET `typeID` = 5843 WHERE `typeID` = 33012;
UPDATE `dunRoomObjects` SET `typeID` = 5844 WHERE `typeID` = 33013;
UPDATE `dunRoomObjects` SET `typeID` = 5845 WHERE `typeID` = 33014;
UPDATE `dunRoomObjects` SET `typeID` = 5861 WHERE `typeID` = 33020;
UPDATE `dunRoomObjects` SET `typeID` = 5862 WHERE `typeID` = 33021;
UPDATE `dunRoomObjects` SET `typeID` = 5863 WHERE `typeID` = 33022;
UPDATE `dunRoomObjects` SET `typeID` = 5864 WHERE `typeID` = 33023;
UPDATE `dunRoomObjects` SET `typeID` = 5865 WHERE `typeID` = 33024;
UPDATE `dunRoomObjects` SET `typeID` = 5866 WHERE `typeID` = 33025;
UPDATE `dunRoomObjects` SET `typeID` = 5867 WHERE `typeID` = 33026;
UPDATE `dunRoomObjects` SET `typeID` = 5868 WHERE `typeID` = 33027;
UPDATE `dunRoomObjects` SET `typeID` = 5869 WHERE `typeID` = 33028;
UPDATE `dunRoomObjects` SET `typeID` = 5870 WHERE `typeID` = 33029;
UPDATE `dunRoomObjects` SET `typeID` = 5871 WHERE `typeID` = 33030;
UPDATE `dunRoomObjects` SET `typeID` = 5872 WHERE `typeID` = 33031;
UPDATE `dunRoomObjects` SET `typeID` = 5873 WHERE `typeID` = 33032;
UPDATE `dunRoomObjects` SET `typeID` = 5878 WHERE `typeID` = 33040;
UPDATE `dunRoomObjects` SET `typeID` = 5879 WHERE `typeID` = 33041;
UPDATE `dunRoomObjects` SET `typeID` = 5880 WHERE `typeID` = 33042;
UPDATE `dunRoomObjects` SET `typeID` = 5881 WHERE `typeID` = 33043;
UPDATE `dunRoomObjects` SET `typeID` = 5882 WHERE `typeID` = 33044;
UPDATE `dunRoomObjects` SET `typeID` = 5883 WHERE `typeID` = 33045;
UPDATE `dunRoomObjects` SET `typeID` = 5884 WHERE `typeID` = 33046;
UPDATE `dunRoomObjects` SET `typeID` = 5885 WHERE `typeID` = 33047;
UPDATE `dunRoomObjects` SET `typeID` = 5886 WHERE `typeID` = 33048;
UPDATE `dunRoomObjects` SET `typeID` = 5887 WHERE `typeID` = 33049;
UPDATE `dunRoomObjects` SET `typeID` = 5888 WHERE `typeID` = 33050;
UPDATE `dunRoomObjects` SET `typeID` = 5889 WHERE `typeID` = 33051;
UPDATE `dunRoomObjects` SET `typeID` = 5890 WHERE `typeID` = 33052;
UPDATE `dunRoomObjects` SET `typeID` = 5891 WHERE `typeID` = 33060;
UPDATE `dunRoomObjects` SET `typeID` = 5892 WHERE `typeID` = 33061;
UPDATE `dunRoomObjects` SET `typeID` = 5893 WHERE `typeID` = 33062;
UPDATE `dunRoomObjects` SET `typeID` = 5894 WHERE `typeID` = 33063;
UPDATE `dunRoomObjects` SET `typeID` = 5895 WHERE `typeID` = 33064;
UPDATE `dunRoomObjects` SET `typeID` = 5896 WHERE `typeID` = 33065;
UPDATE `dunRoomObjects` SET `typeID` = 5897 WHERE `typeID` = 33066;
UPDATE `dunRoomObjects` SET `typeID` = 5898 WHERE `typeID` = 33067;
UPDATE `dunRoomObjects` SET `typeID` = 5899 WHERE `typeID` = 33068;
UPDATE `dunRoomObjects` SET `typeID` = 5900 WHERE `typeID` = 33069;
UPDATE `dunRoomObjects` SET `typeID` = 5901 WHERE `typeID` = 33070;
UPDATE `dunRoomObjects` SET `typeID` = 5902 WHERE `typeID` = 33071;
UPDATE `dunRoomObjects` SET `typeID` = 5903 WHERE `typeID` = 33072;
UPDATE `dunRoomObjects` SET `typeID` = 5905 WHERE `typeID` = 33080;
UPDATE `dunRoomObjects` SET `typeID` = 5906 WHERE `typeID` = 33081;
UPDATE `dunRoomObjects` SET `typeID` = 5907 WHERE `typeID` = 33082;
UPDATE `dunRoomObjects` SET `typeID` = 5908 WHERE `typeID` = 33083;
UPDATE `dunRoomObjects` SET `typeID` = 5909 WHERE `typeID` = 33084;
UPDATE `dunRoomObjects` SET `typeID` = 5910 WHERE `typeID` = 33085;
UPDATE `dunRoomObjects` SET `typeID` = 5915 WHERE `typeID` = 33086;
UPDATE `dunRoomObjects` SET `typeID` = 5916 WHERE `typeID` = 33087;
UPDATE `dunRoomObjects` SET `typeID` = 5917 WHERE `typeID` = 33088;
UPDATE `dunRoomObjects` SET `typeID` = 5918 WHERE `typeID` = 33089;
UPDATE `dunRoomObjects` SET `typeID` = 5919 WHERE `typeID` = 33090;
UPDATE `dunRoomObjects` SET `typeID` = 5911 WHERE `typeID` = 33100;
UPDATE `dunRoomObjects` SET `typeID` = 5912 WHERE `typeID` = 33101;
UPDATE `dunRoomObjects` SET `typeID` = 5913 WHERE `typeID` = 33102;
UPDATE `dunRoomObjects` SET `typeID` = 5914 WHERE `typeID` = 33103;
UPDATE `dunRoomObjects` SET `typeID` = 5921 WHERE `typeID` = 33104;
UPDATE `dunRoomObjects` SET `typeID` = 5922 WHERE `typeID` = 33105;
UPDATE `dunRoomObjects` SET `typeID` = 5923 WHERE `typeID` = 33106;
UPDATE `dunRoomObjects` SET `typeID` = 5924 WHERE `typeID` = 33107;
UPDATE `dunRoomObjects` SET `typeID` = 5925 WHERE `typeID` = 33108;
UPDATE `dunRoomObjects` SET `typeID` = 5931 WHERE `typeID` = 33120;
UPDATE `dunRoomObjects` SET `typeID` = 5932 WHERE `typeID` = 33121;
UPDATE `dunRoomObjects` SET `typeID` = 5933 WHERE `typeID` = 33122;
UPDATE `dunRoomObjects` SET `typeID` = 5934 WHERE `typeID` = 33123;
UPDATE `dunRoomObjects` SET `typeID` = 5935 WHERE `typeID` = 33124;
UPDATE `dunRoomObjects` SET `typeID` = 5936 WHERE `typeID` = 33125;
UPDATE `dunRoomObjects` SET `typeID` = 5937 WHERE `typeID` = 33126;
UPDATE `dunRoomObjects` SET `typeID` = 5938 WHERE `typeID` = 33127;
UPDATE `dunRoomObjects` SET `typeID` = 5939 WHERE `typeID` = 33128;
UPDATE `dunRoomObjects` SET `typeID` = 5941 WHERE `typeID` = 33140;
UPDATE `dunRoomObjects` SET `typeID` = 5942 WHERE `typeID` = 33141;
UPDATE `dunRoomObjects` SET `typeID` = 5943 WHERE `typeID` = 33142;
UPDATE `dunRoomObjects` SET `typeID` = 5944 WHERE `typeID` = 33143;
UPDATE `dunRoomObjects` SET `typeID` = 5945 WHERE `typeID` = 33144;
UPDATE `dunRoomObjects` SET `typeID` = 5946 WHERE `typeID` = 33145;
UPDATE `dunRoomObjects` SET `typeID` = 5947 WHERE `typeID` = 33146;
UPDATE `dunRoomObjects` SET `typeID` = 5948 WHERE `typeID` = 33147;
UPDATE `dunRoomObjects` SET `typeID` = 5949 WHERE `typeID` = 33148;
DELETE FROM `invTypes` WHERE `typeID` BETWEEN 33001 AND 33148;
ALTER TABLE `invTypes` MODIFY `typeID` smallint(5) NOT NULL DEFAULT '0';
