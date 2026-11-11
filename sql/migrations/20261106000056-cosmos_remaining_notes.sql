-- Remaining Minmatar COSMOS descriptions (wiki mission notes)
-- +migrate Up

UPDATE missionObjectives SET briefingText='The mission objectives are found in Contested Minmatar Army Complex.' WHERE missionID=93405 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The mission objectives are found in Contested Minmatar Army Complex.' WHERE missionID=93406 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The mission objectives are found in Contested Minmatar Army Complex.' WHERE missionID=93407 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The mission objectives are found in Contested Minmatar Army Complex.' WHERE missionID=93419 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The mission objective is found in Contested Minmatar Army Complex.' WHERE missionID=93421 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='Although you can destroy Special Forces Command Post structure and pick up mission objective item, the mission will not be marked as complete until you destroy all the rats.' WHERE missionID=93422 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='Simple courier mission. This time you don''t need to go to the Contested Minmatar Guerilla Base.' WHERE missionID=93437 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The agent requests some sleeper components. In return you will get some isk and corporation standing. This mission can be repeated.' WHERE missionID=93525 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The Spatial Analyzers can be found by hacking the containers in Contested Minmatar Army Complex or by purchasing them from the market. This mission gives no faction standings and can be repeated.' WHERE missionID=93526 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='The agent requests some sleeper components. In return you will get some isk and corporation standing. This mission can be repeated.' WHERE missionID=93530 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='This courier mission requires you to have a ship with moderate cargo capacity. A T1 exploration frigate with cargo expanders and MWD+cloak combo is recommended. Remember that you can be attacked in low security space.' WHERE missionID=93531 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='Simple courier mission.' WHERE missionID=93532 AND (briefingText IS NULL OR briefingText='');
UPDATE missionObjectives SET briefingText='Simple courier mission. The items can be bought from market and brought with you if you want to skip the flying part.' WHERE missionID=93533 AND (briefingText IS NULL OR briefingText='');

-- +migrate Down
-- (briefings stay)
