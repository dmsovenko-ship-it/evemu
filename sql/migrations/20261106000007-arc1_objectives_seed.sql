-- +migrate Up
-- Curated Blood-Stained Stars objectives, derived from the client localization
-- texts (tools/extract_localization.py). briefingID makes the agent dialog show
-- the real EVE briefing for the mission; objectiveType drives completion:
--   1 travel, 2 courier, 3 hunt.
INSERT IGNORE INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80005, 140573, 1, 0,    0, 0, 'Jet-Canning a Janitor: find the ship janitor Nebben Centrien'),
  (80006, 140593, 1, 0,    0, 0, 'Chivvying a Chef: track down the ship cook Aubrei Azil'),
  (80007, 140603, 3, 0,    0, 5, 'Delivering a Doctor: assault the Serpentis station, rescue Dr. Castille'),
  (80028, 140641, 2, 3779, 1, 0, 'Planting the Body: fly the body to the Serpentis outpost'),
  (80029, 140694, 3, 0,    0, 5, 'Chasing a Nightmare: find and destroy the drone swarm'),
  (80030, 140619, 3, 0,    0, 5, 'Burning Down the Hive: clear the drone hive'),
  (80040, 140727, 1, 0,    0, 0, 'A Matter of Decorum: speak with the Amarr contact about Dagan');

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN (80005,80006,80007,80028,80029,80030,80040);
