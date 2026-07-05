-- Fix epicArcMission PK to include branchID (allows branching missions)
-- +migrate Up

DROP TABLE IF EXISTS `epicArcMission`;

CREATE TABLE `epicArcMission` (
  `arcID` int(5) NOT NULL DEFAULT 0,
  `chapterNumber` tinyint(2) NOT NULL DEFAULT 0,
  `sequenceNumber` tinyint(2) NOT NULL DEFAULT 0,
  `missionID` int(10) NOT NULL DEFAULT 0,
  `missionName` varchar(100) NOT NULL DEFAULT '',
  `branchID` tinyint(1) NOT NULL DEFAULT 0,
  `rewardISK` int(10) NOT NULL DEFAULT 0,
  `rewardStanding` float NOT NULL DEFAULT 0,
  PRIMARY KEY (`arcID`, `chapterNumber`, `sequenceNumber`, `branchID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- The Blood-Stained Stars: Chapters 1-3 (no branching)
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
(1, 1, 1, 80000, 'A Beacon Beckons', 0, 15000, 0),
(1, 1, 2, 80001, 'Agent Inquiry', 0, 7000, 0),
(1, 1, 3, 80002, 'Of Interest', 0, 7500, 0),
(1, 1, 4, 80003, 'Retrieving Red', 0, 7500, 0),
(1, 1, 5, 80004, 'Alerting Alitura', 0, 7500, 0),
(1, 1, 6, 80005, 'Jet-Canning a Janitor', 0, 15000, 0),
(1, 1, 7, 80006, 'Chivvying a Chef', 0, 7500, 0),
(1, 1, 8, 80007, 'Delivering a Doctor', 0, 7500, 0),
(1, 1, 9, 80008, 'Engineering a Rescue', 0, 15000, 0),
(1, 1, 10, 80009, 'Going Gallente', 0, 7500, 0),
(1, 2, 11, 80010, 'Studying the Scene', 0, 7500, 0),
(1, 2, 12, 80011, 'Rendering Assistance', 0, 7500, 0),
(1, 2, 13, 80012, 'Lair of the Snakes', 0, 15000, 0),
(1, 2, 14, 80013, 'Data Retrieval', 0, 7500, 0),
(1, 2, 15, 80014, 'Crossing Enemy Lines', 0, 15000, 0),
(1, 3, 16, 80015, 'Passive Observation', 0, 7500, 0),
(1, 3, 17, 80016, 'House of Records', 0, 7500, 0),
(1, 3, 18, 80017, 'Mercenary Distractions', 0, 15000, 0),
(1, 3, 19, 80018, 'An Economy Under Threat', 0, 15000, 0),
(1, 3, 20, 80019, 'Every Drone Inside', 0, 15000, 0),
(1, 3, 21, 80020, 'A Sense of Dread', 0, 7500, 0),
(1, 4, 22, 80021, 'Royal Jelly', 0, 7500, 0),
(1, 4, 23, 80022, 'Nature Pictures', 0, 7500, 0),
(1, 4, 24, 80023, 'Tracking or Scanning', 0, 7500, 0),
-- Chapter 4 branch 1: Queen track
(1, 4, 25, 80024, 'Tracking the Queen (Part 1)', 1, 15000, 0),
(1, 4, 26, 80025, 'Tracking the Queen (Part 2)', 1, 15000, 0),
(1, 4, 27, 80026, 'Tracking the Queen (Part 3)', 1, 15000, 0),
-- Chapter 4 branch 2: Blood track
(1, 4, 25, 80027, 'Bag of Blood', 2, 15000, 0),
(1, 4, 26, 80028, 'Planting the Body', 2, 15000, 0),
(1, 4, 27, 80029, 'Chasing a Nightmare', 2, 15000, 0),
-- Chapter 4 merge
(1, 4, 28, 80030, 'Burning Down the Hive', 0, 15000, 0),
(1, 4, 29, 80031, 'It''s Not Over Yet', 0, 7500, 0),
(1, 5, 30, 80032, 'An Eye on Everything', 0, 7500, 0),
(1, 5, 31, 80033, 'The Uses of Force', 0, 15000, 0),
(1, 5, 32, 80034, 'Goading the Leader', 0, 15000, 0),
(1, 5, 33, 80035, 'Hunting the Lieutenants', 0, 15000, 0),
(1, 5, 34, 80036, 'Valuable Cargo', 0, 15000, 0),
(1, 5, 35, 80037, 'Marked for Death', 0, 15000, 0),
(1, 5, 36, 80038, 'Thwarting the Succession', 0, 15000, 0),
(1, 5, 37, 80039, 'Certificate of Death', 0, 15000, 0),
(1, 6, 38, 80040, 'A Matter of Decorum', 0, 7500, 0),
(1, 6, 39, 80041, 'New Friends', 0, 7500, 0),
(1, 6, 40, 80042, 'Recovery', 0, 15000, 0),
(1, 6, 41, 80043, 'Of Quiet Nights Long Past', 0, 7500, 0),
(1, 6, 42, 80044, 'Revelations', 0, 7500, 0),
(1, 6, 43, 80045, 'A Call to Trial', 0, 15000, 0),
(1, 6, 44, 80046, 'Brothers and Sisters', 0, 15000, 0),
(1, 7, 45, 80047, 'A Stranger''s Face', 0, 7500, 0),
(1, 7, 46, 80048, 'The Sisters and the Spy', 0, 7500, 0),
(1, 7, 47, 80049, 'Sealing the Deal', 0, 7500, 0),
(1, 7, 48, 80050, 'Chasing Shadows', 0, 7500, 0),
(1, 7, 49, 80051, 'The Missing Piece', 0, 7500, 0),
-- Chapter 7 branch: Commander choice (1=Amarr, 2=Caldari, 3=Gallente, 4=Minmatar)
(1, 7, 50, 80052, 'The Amarr Commander', 1, 15000, 0),
(1, 7, 50, 80053, 'The Caldari Commander', 2, 15000, 0),
(1, 7, 50, 80054, 'The Gallente Commander', 3, 15000, 0),
(1, 7, 50, 80055, 'The Minmatar Commander', 4, 15000, 0),
-- Final
(1, 7, 51, 80056, 'Our Man Dagan', 0, 50000, 0.7),
(1, 7, 52, 80057, 'Dal Segno Al Fine', 0, 50000, 0);

-- +migrate Down
DROP TABLE IF EXISTS `epicArcMission`;
-- Restore from original 20260705000002-epic_arcs.sql if needed
