-- Epic arc branch missions: real objectives/destinations from EVE-Survival
-- (An Honorable Betrayal / Two Steps into Hell / Everybody Has a Price).
-- +migrate Up
UPDATE missionObjectives SET objectiveType=2, itemTypeID=30955, itemQty=1,
  destinationSystemID=30000187, destinationStationID=60000526,
  objectiveText='Deliver the S.I Formula Sheet to Arikio Kuretsu in Wuos (Wuos V - Moon 1 - Hyasyoda Corporation Mining Outpost).'
 WHERE missionID=80124;
UPDATE missionObjectives SET objectiveType=2, itemTypeID=30955, itemQty=1,
  destinationSystemID=30003343, destinationStationID=0,
  objectiveText='Deliver the S.I Formula Sheet to the Nugoeihuvi Cargo Facility in KFR-ZE (Syndicate).'
 WHERE missionID=80128;
UPDATE missionObjectives SET objectiveType=2, itemTypeID=32292, itemQty=1,
  destinationSystemID=30002659, destinationStationID=60001864
 WHERE missionID=80168;
-- +migrate Down
-- (objectives stay)
