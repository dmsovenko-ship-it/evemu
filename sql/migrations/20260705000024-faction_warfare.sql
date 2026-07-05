-- Faction Warfare agents (agentTypeID=9 = FacWar)
-- 4 militia corps; levels 1-4 each; Security division (1); standalone agents
-- +migrate Up

REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
-- 24th Imperial Crusade (Amarr Empire) — Black Rise / Devoid lowsec
(3019765, 1, 1000179, 60015093, 1, 20, 9, 0),
(3019766, 1, 1000179, 60015093, 2, 20, 9, 0),
(3019767, 1, 1000179, 60015093, 3, 20, 9, 0),
(3019768, 1, 1000179, 60015093, 4, 20, 9, 0),
-- State Protectorate (Caldari State) — Black Rise / Placid lowsec
(3019769, 1, 1000180, 60015093, 1, 20, 9, 0),
(3019770, 1, 1000180, 60015093, 2, 20, 9, 0),
(3019771, 1, 1000180, 60015093, 3, 20, 9, 0),
(3019772, 1, 1000180, 60015093, 4, 20, 9, 0),
-- Federal Defence Union (Gallente Federation) — Placid / Essence lowsec
(3019773, 1, 1000181, 60015093, 1, 20, 9, 0),
(3019774, 1, 1000181, 60015093, 2, 20, 9, 0),
(3019775, 1, 1000181, 60015093, 3, 20, 9, 0),
(3019776, 1, 1000181, 60015093, 4, 20, 9, 0),
-- Tribal Liberation Force (Minmatar Republic) — Heimatar / Metropolis lowsec
(3019777, 1, 1000182, 60015093, 1, 20, 9, 0),
(3019778, 1, 1000182, 60015093, 2, 20, 9, 0),
(3019779, 1, 1000182, 60015093, 3, 20, 9, 0),
(3019780, 1, 1000182, 60015093, 4, 20, 9, 0);

-- +migrate Down
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019765 AND 3019780;
