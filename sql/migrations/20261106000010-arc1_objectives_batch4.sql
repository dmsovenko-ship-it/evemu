-- +migrate Up
-- Blood-Stained Stars objectives batch 4 - remaining confidently identified sites.
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80012, 140871, 3, 0, 0, 5, 'Lair of the Snakes: destroy the Serpentis station and its guards'),
  (80020, 140087, 3, 0, 0, 5, 'A Sense of Dread: destroy the Amarr strike force (dreadnought)')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType),
  npcCount = VALUES(npcCount), note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN (80012,80020);
