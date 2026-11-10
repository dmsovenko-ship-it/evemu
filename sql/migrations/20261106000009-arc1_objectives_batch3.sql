-- +migrate Up
-- Blood-Stained Stars objectives batch 3 - story missions, faction commanders,
-- and the final run (objective/completion pairs 14087x-14099x).
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80010, 140857, 3, 0,    0, 5, 'Studying the Scene: kill the pirates at the disaster site, recover findings'),
  (80033, 140922, 3, 0,    0, 5, 'The Uses of Force: kill all of the mercenary forces'),
  (80034, 130062, 3, 0,    0, 6, 'Goading the Leader: assault the rebel headquarters'),
  (80035, 140924, 3, 0,    0, 5, 'Hunting the Lieutenants: clear the facility, capture lieutenants'),
  (80036, 115541, 3, 0,    0, 3, 'Valuable Cargo: destroy the suspicious craft and retrieve its cargo'),
  (80038, 140922, 3, 0,    0, 5, 'Thwarting the Succession: break the rival succession'),
  (80041, 141235, 1, 0,    0, 0, 'New Friends: contact Keita Eslin and continue the investigation'),
  (80042, 140938, 1, 0,    0, 0, 'Recovery: recover Canius'' artifact'),
  (80043, 140940, 1, 0,    0, 0, 'Of Quiet Nights Long Past: investigate the abandoned monastery'),
  (80044, 140942, 1, 0,    0, 0, 'Revelations: report that Dagan was a spy to Dovirch Anselm'),
  (80046, 140946, 1, 0,    0, 0, 'Brothers and Sisters: confront the Sisters about Dagan'),
  (80048, 140950, 2, 3779, 1, 0, 'The Sisters and the Spy: deliver the false Karin'),
  (80051, 140956, 1, 0,    0, 0, 'The Missing Piece: choose the faction to work with'),
  (80052, 140990, 1, 0,    0, 0, 'The Amarr Commander: meet the Amarr commander'),
  (80053, 140989, 1, 0,    0, 0, 'The Caldari Commander: meet the Caldari commander'),
  (80054, 140985, 1, 0,    0, 0, 'The Gallente Commander: meet Brus Colterne'),
  (80055, 140988, 1, 0,    0, 0, 'The Minmatar Commander: meet the Minmatar commander')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType),
  itemTypeID = VALUES(itemTypeID), itemQty = VALUES(itemQty),
  npcCount = VALUES(npcCount), note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN
  (80010,80033,80034,80035,80036,80038,80041,80042,80043,80044,80046,80048,80051,80052,80053,80054,80055);
