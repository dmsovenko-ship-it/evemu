-- +migrate Up
-- COSMOS: explicit hand-in objectives (provide/trade/deliver to agent) become
-- fetch objectives, with the real item resolved in invTypes. Idempotent.

UPDATE agtMissions SET name='Trade in Angel Diamond Tags' WHERE id=93454;

UPDATE missionObjectives SET objectiveType=5, itemTypeID=23699, itemQty=14 WHERE missionID=93042;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=23936, itemQty=1 WHERE missionID=93076;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=24342, itemQty=1 WHERE missionID=93077;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21601, itemQty=1 WHERE missionID=93152;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21628, itemQty=1 WHERE missionID=93153;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21739, itemQty=1 WHERE missionID=93162;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21464, itemQty=1 WHERE missionID=93163;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21793, itemQty=1 WHERE missionID=93165;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21064, itemQty=1 WHERE missionID=93169;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21066, itemQty=1 WHERE missionID=93170;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21070, itemQty=1 WHERE missionID=93176;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21091, itemQty=1 WHERE missionID=93178;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=22494, itemQty=1 WHERE missionID=93187;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21048, itemQty=1 WHERE missionID=93208;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21734, itemQty=240 WHERE missionID=93221;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21736, itemQty=1 WHERE missionID=93223;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21126, itemQty=30 WHERE missionID=93240;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21465, itemQty=1 WHERE missionID=93243;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=21466, itemQty=3 WHERE missionID=93244;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=20477, itemQty=60 WHERE missionID=93249;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=20480, itemQty=25 WHERE missionID=93251;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=20551, itemQty=1 WHERE missionID=93263;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=20554, itemQty=1 WHERE missionID=93264;
UPDATE missionObjectives SET objectiveType=5, itemTypeID=15648, itemQty=1 WHERE missionID=93576;

-- fetch narratives: keep the wiki objective text in the journal
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93042 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93076 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93077 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93152 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93153 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93162 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93163 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93165 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93169 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93170 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93176 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93178 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93187 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93208 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93221 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93223 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93240 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93243 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93244 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93249 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93251 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93263 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93264 AND (objectiveText IS NULL OR objectiveText='');
UPDATE missionObjectives SET objectiveText=CONCAT('Bring ', itemQty, ' x item ', itemTypeID, ' to the agent.') WHERE missionID=93576 AND (objectiveText IS NULL OR objectiveText='');

-- +migrate Down

UPDATE agtMissions SET name='Mission' WHERE id=93454;

UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93042;
UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93076;
UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93077;
UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93152;
UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93153;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93162;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93163;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93165;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93169;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93170;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93176;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93178;
UPDATE missionObjectives SET objectiveType=1, itemTypeID=0, itemQty=0 WHERE missionID=93187;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93208;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93221;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93223;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93240;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93243;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93244;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93249;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93251;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93263;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93264;
UPDATE missionObjectives SET objectiveType=3, itemTypeID=0, itemQty=0 WHERE missionID=93576;
