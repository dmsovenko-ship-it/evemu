-- COSMOS item hand-in missions: acquire the item and bring it to the agent
-- (objectiveType 5), plus one real station courier.
-- +migrate Up
UPDATE missionObjectives SET objectiveType=5
 WHERE missionID>=93000 AND objectiveType=1 AND itemTypeID>0
   AND (objectiveText LIKE '%to agent%' OR objectiveText LIKE 'Provide%'
        OR objectiveText LIKE 'Acquire%' OR objectiveText LIKE 'Retrieve%'
        OR objectiveText LIKE 'Bring%');
UPDATE missionObjectives SET objectiveType=2, destinationSystemID=30004087, destinationStationID=60008629
 WHERE missionID=93001;
-- +migrate Down
-- (objectives stay)
