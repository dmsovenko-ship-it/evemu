-- Mission encounter dungeons by faction and level
-- +migrate Up

-- Helper: ensure dunRoomObjects auto_increment starts high
-- Dungeon IDs: 1000-1999 = faction encounter templates

-- ====== GURISTAS (faction 500014) ======
-- Level 1: 4 frigates + 1 destroyer
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1000, 'Guristas Encounter L1', 3, 500014, 1, 'guristas-enc-l1'),
(1001, 'Guristas Encounter L2', 3, 500014, 1, 'guristas-enc-l2'),
(1002, 'Guristas Encounter L3', 3, 500014, 1, 'guristas-enc-l3'),
(1003, 'Guristas Encounter L4', 3, 500014, 1, 'guristas-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1000, 'Combat Pocket', 1000),
(1001, 'Combat Pocket', 1001),
(1002, 'Combat Pocket', 1002),
(1003, 'Combat Pocket', 1003);

-- Guristas L1: 4x frigate (5832-5834), 1x destroyer (5835)
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1000, 5832, 0, 0, 0, 1500, 0, 0, 0, 0),
(1000, 5833, 0, 1300, 0, -750, 0, 0, 0, 0),
(1000, 5834, 0, -1300, 0, -750, 0, 0, 0, 0),
(1000, 5835, 0, 0, 0, -3000, 0, 0, 0, 0);
-- Guristas L2: 3x frigate, 2x cruiser
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1001, 5836, 0, 0, 0, 2000, 0, 0, 0, 0),
(1001, 5837, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1001, 5838, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1001, 5839, 0, 0, 0, -3500, 0, 0, 0, 0);
-- Guristas L3: 2x cruiser, 2x battleship
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1002, 5840, 0, 0, 0, 2500, 0, 0, 0, 0),
(1002, 5840, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1002, 5841, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1002, 5842, 0, 0, 0, -4000, 0, 0, 0, 0);
-- Guristas L4: 3x battleship, 1x commander
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1003, 5843, 0, 0, 0, 3000, 0, 0, 0, 0),
(1003, 5844, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1003, 5844, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1003, 5845, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== ANGEL CARTEL (faction 500011) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1010, 'Angel Encounter L1', 3, 500011, 1, 'angel-enc-l1'),
(1011, 'Angel Encounter L2', 3, 500011, 1, 'angel-enc-l2'),
(1012, 'Angel Encounter L3', 3, 500011, 1, 'angel-enc-l3'),
(1013, 'Angel Encounter L4', 3, 500011, 1, 'angel-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1010, 'Combat Pocket', 1010),
(1011, 'Combat Pocket', 1011),
(1012, 'Combat Pocket', 1012),
(1013, 'Combat Pocket', 1013);

-- Angel L1
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1010, 5861, 0, 0, 0, 1500, 0, 0, 0, 0),
(1010, 5862, 0, 1300, 0, -750, 0, 0, 0, 0),
(1010, 5863, 0, -1300, 0, -750, 0, 0, 0, 0),
(1010, 5864, 0, 0, 0, -3000, 0, 0, 0, 0);
-- Angel L2
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1011, 5865, 0, 0, 0, 2000, 0, 0, 0, 0),
(1011, 5866, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1011, 5867, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1011, 5865, 0, 0, 0, -3500, 0, 0, 0, 0);
-- Angel L3
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1012, 5868, 0, 0, 0, 2500, 0, 0, 0, 0),
(1012, 5868, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1012, 5869, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1012, 5870, 0, 0, 0, -4000, 0, 0, 0, 0);
-- Angel L4
INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1013, 5871, 0, 0, 0, 3000, 0, 0, 0, 0),
(1013, 5872, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1013, 5872, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1013, 5873, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== BLOOD RAIDERS (faction 500012) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1020, 'Blood Encounter L1', 3, 500012, 1, 'blood-enc-l1'),
(1021, 'Blood Encounter L2', 3, 500012, 1, 'blood-enc-l2'),
(1022, 'Blood Encounter L3', 3, 500012, 1, 'blood-enc-l3'),
(1023, 'Blood Encounter L4', 3, 500012, 1, 'blood-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1020, 'Combat Pocket', 1020),
(1021, 'Combat Pocket', 1021),
(1022, 'Combat Pocket', 1022),
(1023, 'Combat Pocket', 1023);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1020, 5891, 0, 0, 0, 1500, 0, 0, 0, 0),
(1020, 5892, 0, 1300, 0, -750, 0, 0, 0, 0),
(1020, 5893, 0, -1300, 0, -750, 0, 0, 0, 0),
(1020, 5894, 0, 0, 0, -3000, 0, 0, 0, 0),
(1021, 5895, 0, 0, 0, 2000, 0, 0, 0, 0),
(1021, 5896, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1021, 5897, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1021, 5895, 0, 0, 0, -3500, 0, 0, 0, 0),
(1022, 5898, 0, 0, 0, 2500, 0, 0, 0, 0),
(1022, 5898, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1022, 5899, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1022, 5900, 0, 0, 0, -4000, 0, 0, 0, 0),
(1023, 5901, 0, 0, 0, 3000, 0, 0, 0, 0),
(1023, 5902, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1023, 5902, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1023, 5903, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== SERPENTIS (faction 500013) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1030, 'Serpentis Encounter L1', 3, 500013, 1, 'serp-enc-l1'),
(1031, 'Serpentis Encounter L2', 3, 500013, 1, 'serp-enc-l2'),
(1032, 'Serpentis Encounter L3', 3, 500013, 1, 'serp-enc-l3'),
(1033, 'Serpentis Encounter L4', 3, 500013, 1, 'serp-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1030, 'Combat Pocket', 1030),
(1031, 'Combat Pocket', 1031),
(1032, 'Combat Pocket', 1032),
(1033, 'Combat Pocket', 1033);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1030, 5878, 0, 0, 0, 1500, 0, 0, 0, 0),
(1030, 5879, 0, 1300, 0, -750, 0, 0, 0, 0),
(1030, 5880, 0, -1300, 0, -750, 0, 0, 0, 0),
(1030, 5881, 0, 0, 0, -3000, 0, 0, 0, 0),
(1031, 5882, 0, 0, 0, 2000, 0, 0, 0, 0),
(1031, 5883, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1031, 5884, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1031, 5882, 0, 0, 0, -3500, 0, 0, 0, 0),
(1032, 5885, 0, 0, 0, 2500, 0, 0, 0, 0),
(1032, 5885, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1032, 5886, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1032, 5887, 0, 0, 0, -4000, 0, 0, 0, 0),
(1033, 5888, 0, 0, 0, 3000, 0, 0, 0, 0),
(1033, 5889, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1033, 5889, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1033, 5890, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== SANSHA (faction 500019) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1040, 'Sansha Encounter L1', 3, 500019, 1, 'sansha-enc-l1'),
(1041, 'Sansha Encounter L2', 3, 500019, 1, 'sansha-enc-l2'),
(1042, 'Sansha Encounter L3', 3, 500019, 1, 'sansha-enc-l3'),
(1043, 'Sansha Encounter L4', 3, 500019, 1, 'sansha-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1040, 'Combat Pocket', 1040),
(1041, 'Combat Pocket', 1041),
(1042, 'Combat Pocket', 1042),
(1043, 'Combat Pocket', 1043);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1040, 5905, 0, 0, 0, 1500, 0, 0, 0, 0),
(1040, 5906, 0, 1300, 0, -750, 0, 0, 0, 0),
(1040, 5907, 0, -1300, 0, -750, 0, 0, 0, 0),
(1040, 5908, 0, 0, 0, -3000, 0, 0, 0, 0),
(1041, 5909, 0, 0, 0, 2000, 0, 0, 0, 0),
(1041, 5910, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1041, 5909, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1041, 5910, 0, 0, 0, -3500, 0, 0, 0, 0),
(1042, 5915, 0, 0, 0, 2500, 0, 0, 0, 0),
(1042, 5915, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1042, 5916, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1042, 5916, 0, 0, 0, -4000, 0, 0, 0, 0),
(1043, 5917, 0, 0, 0, 3000, 0, 0, 0, 0),
(1043, 5918, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1043, 5918, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1043, 5919, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== ROGUE DRONES (faction 500020) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1050, 'Rogue Drone Encounter L1', 3, 500020, 1, 'drone-enc-l1'),
(1051, 'Rogue Drone Encounter L2', 3, 500020, 1, 'drone-enc-l2'),
(1052, 'Rogue Drone Encounter L3', 3, 500020, 1, 'drone-enc-l3'),
(1053, 'Rogue Drone Encounter L4', 3, 500020, 1, 'drone-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1050, 'Combat Pocket', 1050),
(1051, 'Combat Pocket', 1051),
(1052, 'Combat Pocket', 1052),
(1053, 'Combat Pocket', 1053);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1050, 5911, 0, 0, 0, 1500, 0, 0, 0, 0),
(1050, 5912, 0, 1300, 0, -750, 0, 0, 0, 0),
(1050, 5913, 0, -1300, 0, -750, 0, 0, 0, 0),
(1050, 5914, 0, 0, 0, -3000, 0, 0, 0, 0),
(1051, 5921, 0, 0, 0, 2000, 0, 0, 0, 0),
(1051, 5922, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1051, 5921, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1051, 5922, 0, 0, 0, -3500, 0, 0, 0, 0),
(1052, 5923, 0, 0, 0, 2500, 0, 0, 0, 0),
(1052, 5923, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1052, 5923, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1052, 5924, 0, 0, 0, -4000, 0, 0, 0, 0),
(1053, 5924, 0, 0, 0, 3000, 0, 0, 0, 0),
(1053, 5924, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1053, 5924, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1053, 5925, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== MORDU'S LEGION (faction 500010) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1060, 'Mordus Encounter L1', 3, 500010, 1, 'mordus-enc-l1'),
(1061, 'Mordus Encounter L2', 3, 500010, 1, 'mordus-enc-l2'),
(1062, 'Mordus Encounter L3', 3, 500010, 1, 'mordus-enc-l3'),
(1063, 'Mordus Encounter L4', 3, 500010, 1, 'mordus-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1060, 'Combat Pocket', 1060),
(1061, 'Combat Pocket', 1061),
(1062, 'Combat Pocket', 1062),
(1063, 'Combat Pocket', 1063);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1060, 5931, 0, 0, 0, 1500, 0, 0, 0, 0),
(1060, 5932, 0, 1300, 0, -750, 0, 0, 0, 0),
(1060, 5932, 0, -1300, 0, -750, 0, 0, 0, 0),
(1060, 5933, 0, 0, 0, -3000, 0, 0, 0, 0),
(1061, 5934, 0, 0, 0, 2000, 0, 0, 0, 0),
(1061, 5934, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1061, 5935, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1061, 5935, 0, 0, 0, -3500, 0, 0, 0, 0),
(1062, 5936, 0, 0, 0, 2500, 0, 0, 0, 0),
(1062, 5936, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1062, 5937, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1062, 5937, 0, 0, 0, -4000, 0, 0, 0, 0),
(1063, 5938, 0, 0, 0, 3000, 0, 0, 0, 0),
(1063, 5938, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1063, 5938, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1063, 5939, 0, 0, 0, -5000, 0, 0, 0, 0);

-- ====== CONCORD / DED (faction 500021) ======
INSERT INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(1070, 'DED Encounter L1', 3, 500021, 1, 'ded-enc-l1'),
(1071, 'DED Encounter L2', 3, 500021, 1, 'ded-enc-l2'),
(1072, 'DED Encounter L3', 3, 500021, 1, 'ded-enc-l3'),
(1073, 'DED Encounter L4', 3, 500021, 1, 'ded-enc-l4');

INSERT INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(1070, 'Combat Pocket', 1070),
(1071, 'Combat Pocket', 1071),
(1072, 'Combat Pocket', 1072),
(1073, 'Combat Pocket', 1073);

INSERT INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`, `yaw`, `pitch`, `roll`, `radius`) VALUES
(1070, 5941, 0, 0, 0, 1500, 0, 0, 0, 0),
(1070, 5942, 0, 1300, 0, -750, 0, 0, 0, 0),
(1070, 5942, 0, -1300, 0, -750, 0, 0, 0, 0),
(1070, 5943, 0, 0, 0, -3000, 0, 0, 0, 0),
(1071, 5944, 0, 0, 0, 2000, 0, 0, 0, 0),
(1071, 5944, 0, 1500, 0, -1000, 0, 0, 0, 0),
(1071, 5945, 0, -1500, 0, -1000, 0, 0, 0, 0),
(1071, 5945, 0, 0, 0, -3500, 0, 0, 0, 0),
(1072, 5946, 0, 0, 0, 2500, 0, 0, 0, 0),
(1072, 5946, 0, 2000, 0, -1200, 0, 0, 0, 0),
(1072, 5947, 0, -2000, 0, -1200, 0, 0, 0, 0),
(1072, 5947, 0, 0, 0, -4000, 0, 0, 0, 0),
(1073, 5948, 0, 0, 0, 3000, 0, 0, 0, 0),
(1073, 5948, 0, 2500, 0, -1500, 0, 0, 0, 0),
(1073, 5948, 0, -2500, 0, -1500, 0, 0, 0, 0),
(1073, 5949, 0, 0, 0, -5000, 0, 0, 0, 0);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 1000 AND 1073;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 1000 AND 1073;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 1000 AND 1073;
