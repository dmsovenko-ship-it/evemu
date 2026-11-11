-- Blood Raiders COSMOS hunt missions in Delve (region has no faction row).
-- +migrate Up
UPDATE missionObjectives SET npcFactionID = 500012
 WHERE missionID IN (93144, 93145) AND (npcFactionID IS NULL OR npcFactionID = 0);

-- +migrate Down
-- (faction hints stay)
