-- COSMOS courier missions with a real destination station -> type 2 (courier)
-- +migrate Up
UPDATE missionObjectives SET objectiveType=2
 WHERE missionID>=93000 AND objectiveType=1 AND itemTypeID>0 AND destinationStationID>0
   AND (objectiveText LIKE 'Transport%' OR objectiveText LIKE 'Deliver%' OR objectiveText LIKE 'Bring%');
-- +migrate Down
-- (objectives stay)
