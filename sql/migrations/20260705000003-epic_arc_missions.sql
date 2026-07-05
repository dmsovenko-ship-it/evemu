-- The Blood-Stained Stars - mission entries + dialog + rewards
-- +migrate Up

-- 52 missions for The Blood-Stained Stars (typeID=10 = EpicArc, level=1)
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
-- Chapter 1: Quality of Mercy
(80000, 0, 'A Beacon Beckons', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80001, 0, 'Agent Inquiry', 1, 10, 0, 0, 0, 0, 0, 0, 7000, 0, 0, 1500, 0),
(80002, 0, 'Of Interest', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80003, 0, 'Retrieving Red', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80004, 0, 'Alerting Alitura', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80005, 0, 'Jet-Canning a Janitor', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80006, 0, 'Chivvying a Chef', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80007, 0, 'Delivering a Doctor', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80008, 0, 'Engineering a Rescue', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80009, 0, 'Going Gallente', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
-- Chapter 2: Automaton Impediment
(80010, 0, 'Studying the Scene', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80011, 0, 'Rendering Assistance', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80012, 0, 'Lair of the Snakes', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80013, 0, 'Data Retrieval', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80014, 0, 'Crossing Enemy Lines', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Chapter 3: Shadow Puppets
(80015, 0, 'Passive Observation', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80016, 0, 'House of Records', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80017, 0, 'Mercenary Distractions', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80018, 0, 'An Economy Under Threat', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80019, 0, 'Every Drone Inside', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80020, 0, 'A Sense of Dread', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
-- Chapter 4: Queens And Drones
(80021, 0, 'Royal Jelly', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80022, 0, 'Nature Pictures', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80023, 0, 'Tracking or Scanning', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80024, 0, 'Tracking the Queen (Part 1)', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80025, 0, 'Tracking the Queen (Part 2)', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80026, 0, 'Tracking the Queen (Part 3)', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80027, 0, 'Bag of Blood', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80028, 0, 'Planting the Body', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80029, 0, 'Chasing a Nightmare', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80030, 0, 'Burning Down the Hive', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80031, 0, 'It''s Not Over Yet', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
-- Chapter 5: Shifting Foundations
(80032, 0, 'An Eye on Everything', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80033, 0, 'The Uses of Force', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80034, 0, 'Goading the Leader', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80035, 0, 'Hunting the Lieutenants', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80036, 0, 'Valuable Cargo', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80037, 0, 'Marked for Death', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80038, 0, 'Thwarting the Succession', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80039, 0, 'Certificate of Death', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Chapter 6: A Breach of Trust
(80040, 0, 'A Matter of Decorum', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80041, 0, 'New Friends', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80042, 0, 'Recovery', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80043, 0, 'Of Quiet Nights Long Past', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80044, 0, 'Revelations', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80045, 0, 'A Call to Trial', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80046, 0, 'Brothers and Sisters', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Chapter 7: Closing In
(80047, 0, 'A Stranger''s Face', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80048, 0, 'The Sisters and the Spy', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80049, 0, 'Sealing the Deal', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80050, 0, 'Chasing Shadows', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
(80051, 0, 'The Missing Piece', 1, 10, 0, 0, 0, 0, 0, 0, 7500, 0, 0, 1500, 0),
-- Branch (50a-d - same mission, different choice)
(80052, 0, 'The Amarr Commander', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80053, 0, 'The Caldari Commander', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80054, 0, 'The Gallente Commander', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80055, 0, 'The Minmatar Commander', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Final mission
(80056, 0, 'Our Man Dagan', 1, 10, 0, 0, 0, 0, 0, 0, 50000, 0, 0, 10000, 0),
-- Dal Segno Al Fine (epilogue)
(80057, 0, 'Dal Segno Al Fine', 1, 10, 0, 0, 0, 0, 0, 0, 50000, 0, 0, 10000, 0);

-- Update epicArcMission to use real missionIDs
-- Sequence 1-9 -> 80000-80009, 10-19 -> 80010-80019, etc.
UPDATE `epicArcMission` SET `missionID` = 80000 + `sequenceNumber` WHERE `arcID` = 1 AND `missionID` = 0;
-- Fix the branch missions (25-27 have two branches)
UPDATE `epicArcMission` SET `missionID` = 80024 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 25 AND `branchID` = 1;
UPDATE `epicArcMission` SET `missionID` = 80025 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 26 AND `branchID` = 1;
UPDATE `epicArcMission` SET `missionID` = 80026 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 27 AND `branchID` = 1;
UPDATE `epicArcMission` SET `missionID` = 80027 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 25 AND `branchID` = 2;
UPDATE `epicArcMission` SET `missionID` = 80028 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 26 AND `branchID` = 2;
UPDATE `epicArcMission` SET `missionID` = 80029 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 27 AND `branchID` = 2;
UPDATE `epicArcMission` SET `missionID` = 80030 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 28;
UPDATE `epicArcMission` SET `missionID` = 80031 WHERE `arcID` = 1 AND `chapterNumber` = 4 AND `sequenceNumber` = 29;
-- Fix the commander branch (mission 50)
UPDATE `epicArcMission` SET `missionID` = 80052 WHERE `arcID` = 1 AND `chapterNumber` = 7 AND `sequenceNumber` = 50 AND `branchID` = 1;
UPDATE `epicArcMission` SET `missionID` = 80053 WHERE `arcID` = 1 AND `chapterNumber` = 7 AND `sequenceNumber` = 50 AND `branchID` = 2;
UPDATE `epicArcMission` SET `missionID` = 80054 WHERE `arcID` = 1 AND `chapterNumber` = 7 AND `sequenceNumber` = 50 AND `branchID` = 3;
UPDATE `epicArcMission` SET `missionID` = 80055 WHERE `arcID` = 1 AND `chapterNumber` = 7 AND `sequenceNumber` = 50 AND `branchID` = 4;
UPDATE `epicArcMission` SET `missionID` = 80056 WHERE `arcID` = 1 AND `chapterNumber` = 7 AND `sequenceNumber` = 51;
-- Add epilogue (Dal Segno Al Fine)
DELETE FROM `epicArcMission` WHERE `arcID` = 1 AND `sequenceNumber` = 52;
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`)
VALUES (1, 7, 52, 80057, 'Dal Segno Al Fine', 0, 50000, 0.7);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 80000 AND 80057;
UPDATE `epicArcMission` SET `missionID` = 0 WHERE `arcID` = 1;
DELETE FROM `epicArcMission` WHERE `arcID` = 1 AND `sequenceNumber` = 52;
