-- +migrate Up
-- Other epic arcs: a few briefings matched with high confidence against the
-- localization texts (mission name appears verbatim in the briefing and the
-- content matches the mission). Remaining arc missions still use the runtime
-- name-keyword classifier.
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80077, 142516, 1, 0, 0, 0, 'Enemy of my Enemy: meet the Federation Navy contact'),
  (80087, 114809, 3, 0, 0, 3, 'Knockout Punch: take the shuttle and the explosives, hit the target'),
  (80126, 141907, 1, 0, 0, 0, 'Return to Isha: return and earn Arikio''s trust'),
  (80133, 141911, 1, 0, 0, 0, 'Meet Sinas: travel to Astoh, Black Rise and meet Sinas'),
  (80137, 141818, 1, 0, 0, 0, 'Practical Solutions: work with Katsen''s arrangements'),
  (80146, 141499, 1, 0, 0, 0, 'Impetus: meet the contact')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType),
  note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN (80077,80087,80126,80133,80137,80146);
