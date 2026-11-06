-- +migrate Up
-- Curated mission objectives derived from the client localization texts
-- (tools/extract_localization.py + tools/build_mission_objectives.py).
-- Rows take precedence over the runtime name-keyword classifier in
-- MissionDataMgr::AssignArcObjective. briefingID != 0 makes the agent dialog show
-- the real EVE briefing text for that mission.
CREATE TABLE IF NOT EXISTS `missionObjectives` (
  `missionID` int(10) unsigned NOT NULL,
  `briefingID` int(10) unsigned NOT NULL DEFAULT 0,
  `objectiveType` tinyint(4) NOT NULL DEFAULT 0,   -- 1 travel, 2 courier, 3 hunt
  `itemTypeID` int(10) unsigned NOT NULL DEFAULT 0,
  `itemQty` int(10) unsigned NOT NULL DEFAULT 0,
  `npcCount` int(10) unsigned NOT NULL DEFAULT 0,
  `note` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`missionID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS `missionObjectives`;
