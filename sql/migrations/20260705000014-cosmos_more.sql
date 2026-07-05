-- COSMOS: Araz (Amarr), Okkelen (Caldari), Algintal (Gallente) agents + missions
-- +migrate Up

-- === Araz constellation (Amarr COSMOS) ===
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019555, 24, 1000182, 60015093, 4, 20, 12, 0),
(3019556, 24, 1000182, 60015093, 6, 20, 12, 0),
(3019557, 24, 1000182, 60015093, 2, 20, 12, 0),
(3019558, 24, 1000182, 60015093, 4, 20, 12, 0),
(3019559, 24, 1000182, 60015093, 6, 20, 12, 0),
(3019560, 24, 1000182, 60015093, 1, 20, 12, 0),
-- === Okkelen constellation (Caldari COSMOS) ===
(3019561, 24, 1000182, 60015093, 4, 20, 12, 0),
(3019562, 24, 1000182, 60015093, 6, 20, 12, 0),
(3019563, 24, 1000182, 60015093, 2, 20, 12, 0),
(3019564, 24, 1000182, 60015093, 4, 20, 12, 0),
-- === Algintal constellation (Gallente COSMOS) ===
(3019565, 24, 1000182, 60015093, 4, 20, 12, 0),
(3019566, 24, 1000182, 60015093, 6, 20, 12, 0),
(3019567, 24, 1000182, 60015093, 2, 20, 12, 0),
(3019568, 24, 1000182, 60015093, 4, 20, 12, 0),
(3019569, 24, 1000182, 60015093, 1, 20, 12, 0);

-- COSMOS missions
INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
-- Araz (Amarr)
(92120, 0, 'Araz C: Scripture Translation', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92121, 0, 'Araz C: Heretic Cache Excavation', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92122, 0, 'Araz C: Cathedral Archives', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(92123, 0, 'Araz C: Imperial Survey Data', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92124, 0, 'Araz C: Relic Recovery', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(92125, 0, 'Araz C: Minor Artifacts', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Okkelen (Caldari)
(92126, 0, 'Okkelen C: Corporate Espionage', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92127, 0, 'Okkelen C: Prototype Salvage', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92128, 0, 'Okkelen C: Encrypted Data Core', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(92129, 0, 'Okkelen C: Competitor Intel', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Algintal (Gallente)
(92130, 0, 'Algintal C: Luxury Goods Run', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92131, 0, 'Algintal C: Federal Intelligence', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92132, 0, 'Algintal C: Secret Dispatch', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(92133, 0, 'Algintal C: Presidential Decree', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(92134, 0, 'Algintal C: Propaganda Analysis', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92120 AND 92134;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019555 AND 3019569;
