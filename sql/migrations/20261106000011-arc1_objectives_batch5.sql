-- +migrate Up
-- Blood-Stained Stars batch 5: the 12 faction-branch missions have no localized
-- briefing in the client (only journal titles), so give them a deterministic
-- objective by mission meaning (no briefingID). This completes 58/58 arc-1 rows.
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80001, 0, 1, 0,    0, 0, 'Agent Inquiry: ask around and report back (no localized briefing)'),
  (80011, 0, 1, 0,    0, 0, 'Rendering Assistance: help the contact (no localized briefing)'),
  (80014, 0, 1, 0,    0, 0, 'Crossing Enemy Lines: travel through hostile space (no localized briefing)'),
  (80021, 0, 3, 0,    0, 5, 'Royal Jelly: clear the hive guardians (branch mission)'),
  (80022, 0, 1, 0,    0, 0, 'Nature Pictures: survey the site (branch mission)'),
  (80024, 0, 3, 0,    0, 4, 'Tracking the Queen (1): hunt the drone queen''s trail'),
  (80025, 0, 3, 0,    0, 5, 'Tracking the Queen (2): continue the hunt'),
  (80026, 0, 3, 0,    0, 6, 'Tracking the Queen (3): finish the queen'),
  (80027, 0, 2, 3779, 1, 0, 'Bag of Blood: bring the blood bag to the agent'),
  (80031, 0, 3, 0,    0, 5, 'It''s Not Over Yet: chase down the escaped commander'),
  (80032, 0, 1, 0,    0, 0, 'An Eye on Everything: observe the target location'),
  (80037, 0, 3, 0,    0, 5, 'Marked for Death: eliminate the marked target')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType),
  itemTypeID = VALUES(itemTypeID), itemQty = VALUES(itemQty),
  npcCount = VALUES(npcCount), note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN
  (80001,80011,80014,80021,80022,80024,80025,80026,80027,80031,80032,80037);
