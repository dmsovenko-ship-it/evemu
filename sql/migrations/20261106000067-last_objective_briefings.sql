-- Last two missions have no external briefing page: expose the objective.
-- +migrate Up
UPDATE missionObjectives SET briefingText='Objective: Destroy the hostile ships.' WHERE missionID=60100 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='Objective: Trade in 30x Angel Diamond Tag.' WHERE missionID=93454 AND (briefingText IS NULL OR briefingText='');

-- +migrate Down
-- (text stays)
