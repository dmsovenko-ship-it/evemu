-- COSMOS Infiltration/Azure Canyon item hand-ins: the wiki has no station
-- destination, but the mission expects the goods at the agent.
-- +migrate Up
UPDATE missionObjectives SET objectiveType=5
 WHERE missionID IN (93196, 93197, 93332) AND itemTypeID<>0;

-- +migrate Down
-- (objective assignments stay)
