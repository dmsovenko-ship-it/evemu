-- +migrate Up
-- Angel Sound (arc 3): briefings identified by content in the localization block
-- 1428xx (Egbinger / Angel Cartel / Yukiro Demense).
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80058, 142811, 1, 0, 0, 0, 'The Balance of Power: fly to Curse and check the Cartel situation'),
  (80061, 142825, 1, 0, 0, 0, 'New Opportunities: meet the Angel Cartel recruiter Ellar Stin'),
  (80064, 142843, 3, 0, 0, 5, 'Ride to the Rescue: break Yukiro Demense''s clone facility')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType), note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN (80058,80061,80064);
