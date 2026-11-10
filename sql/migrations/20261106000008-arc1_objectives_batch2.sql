-- +migrate Up
-- Blood-Stained Stars objectives batch 2 - matched against the localization
-- "objective / completion" text pairs (1408xx-14096x) and the objective-strip
-- block (1155xx). briefingID drives the agent dialog text; objectiveType drives
-- completion (1 travel, 2 courier, 3 hunt). UPSERT so earlier seeds get refined.
INSERT INTO `missionObjectives`
  (`missionID`, `briefingID`, `objectiveType`, `itemTypeID`, `itemQty`, `npcCount`, `note`) VALUES
  (80000, 140582, 1, 0,     0,   0, 'A Beacon Beckons: deploy the transponder beacon at the site'),
  (80002, 140982, 1, 0,     0,   0, 'Of Interest: investigate the distress call from the Damsel'),
  (80003, 141028, 1, 0,     0,   0, 'Retrieving Red: find the man named Red (via Tevis Jak)'),
  (80004, 141031, 1, 0,     0,   0, 'Alerting Alitura: return to Sister Alitura with the news'),
  (80005, 140573, 1, 0,     0,   0, 'Jet-Canning a Janitor: retrieve Nebben Centrien (obj 141013)'),
  (80006, 140593, 1, 0,     0,   0, 'Chivvying a Chef: rescue Aubrei Azil (obj 140858)'),
  (80007, 140603, 3, 0,     0,   5, 'Delivering a Doctor: destroy the Serpentis station, rescue Dr. Castille'),
  (80008, 140900, 3, 0,     0,   5, 'Engineering a Rescue: rescue Corin Risia (obj 140902)'),
  (80009, 0,      2, 30816, 10,  0, 'Going Gallente: deliver medical supplies to the colony (obj 140869)'),
  (80013, 140577, 1, 0,     0,   0, 'Data Retrieval: find the datacore at the site and bring it back'),
  (80015, 140875, 1, 0,     0,   0, 'Passive Observation: scout the drone hive'),
  (80016, 140877, 1, 0,     0,   0, 'House of Records: visit the archives, get the commander file'),
  (80017, 140879, 3, 0,     0,   5, 'Mercenary Distractions: kill the Guristas mercenaries'),
  (80018, 140881, 2, 30794, 100, 0, 'An Economy Under Threat: deliver farming supplies to the warehouse'),
  (80019, 140883, 3, 0,     0,   6, 'Every Drone Inside: neutralize the drones in the quarantine area'),
  (80023, 140912, 1, 0,     0,   0, 'Tracking or Scanning: check the hotspots the drone was sighted at'),
  (80028, 140641, 2, 3779,  1,   0, 'Planting the Body: plant the fake body in the outpost (obj 115583)'),
  (80029, 140694, 3, 0,     0,   5, 'Chasing a Nightmare: destroy the drone swarm'),
  (80030, 140643, 3, 0,     0,   6, 'Burning Down the Hive: destroy the drone hive'),
  (80039, 140932, 1, 0,     0,   0, 'Certificate of Death: inform the Amarr of Mizara''s death'),
  (80045, 140944, 3, 0,     0,   1, 'A Call to Trial: defeat Taphos in trial by combat'),
  (80047, 140948, 2, 3812,  1,   0, 'A Stranger''s Face: plant the false records'),
  (80049, 140952, 3, 0,     0,   1, 'Sealing the Deal: destroy the Society spy cruiser'),
  (80050, 140954, 3, 0,     0,   5, 'Chasing Shadows: destroy the squadron, capture the leader'),
  (80056, 140958, 3, 0,     0,   5, 'Our Man Dagan: destroy Dagan''s ship and capture him'),
  (80057, 141000, 1, 0,     0,   0, 'Dal Segno Al Fine: deliver Dagan to the Sisters of EVE ship')
ON DUPLICATE KEY UPDATE
  briefingID = VALUES(briefingID), objectiveType = VALUES(objectiveType),
  itemTypeID = VALUES(itemTypeID), itemQty = VALUES(itemQty),
  npcCount = VALUES(npcCount), note = VALUES(note);

-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` IN
  (80000,80002,80003,80004,80005,80006,80007,80008,80009,80013,80015,80016,80017,80018,
   80019,80023,80028,80029,80030,80039,80045,80047,80049,80050,80056,80057);
