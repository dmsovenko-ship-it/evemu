-- COSMOS item missions: resolve the hand-in type/destination
-- (fetch = bring item to the agent; courier = deliver to a known station).
-- +migrate Up

UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93002 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93005 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93008 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93015 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93068 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93073 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93086 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93089 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93126 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93159 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93160 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93161 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93173 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93250 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93253 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93327 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93331 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93353 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93362 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93365 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93383 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93408 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93409 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93417 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93433 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93439 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93448 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93455 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93456 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93457 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93494 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93495 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93500 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93504 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93508 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93513 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93515 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93522 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93523 AND objectiveType=1 AND itemTypeID<>0;
UPDATE missionObjectives SET objectiveType=5, destinationAgentID=0 WHERE missionID=93527 AND objectiveType=1 AND itemTypeID<>0;

UPDATE missionObjectives SET objectiveType=2, destinationStationID=60002239, destinationSystemID=30000194, destinationAgentID=0 WHERE missionID=93184 AND objectiveType=1;
UPDATE missionObjectives SET objectiveType=2, destinationStationID=60001879, destinationSystemID=30000167, destinationAgentID=0 WHERE missionID=93220 AND objectiveType=1;
UPDATE missionObjectives SET objectiveType=2, destinationStationID=60002290, destinationSystemID=30000171, destinationAgentID=0 WHERE missionID=93231 AND objectiveType=1;
UPDATE missionObjectives SET objectiveType=2, destinationStationID=60001882, destinationSystemID=30000168, destinationAgentID=0 WHERE missionID=93232 AND objectiveType=1;
UPDATE missionObjectives SET objectiveType=2, destinationStationID=60001264, destinationSystemID=30002676, destinationAgentID=0 WHERE missionID=93359 AND objectiveType=1;
UPDATE missionObjectives SET objectiveType=2, destinationStationID=60012124, destinationSystemID=30000005, destinationAgentID=0 WHERE missionID=93520 AND objectiveType=1;

-- +migrate Down
-- (objective assignments stay)
