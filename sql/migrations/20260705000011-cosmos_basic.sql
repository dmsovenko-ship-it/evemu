-- COSMOS missions — basic skeleton (one-time chains, unique agents)
-- +migrate Up

INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
-- Ani constellation (Minmatar COSMOS)
(92000, 0, 'Ani COSMOS: Ore Analysis', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92001, 0, 'Ani COSMOS: Tribal Artifacts', 1, 9, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(92002, 0, 'Ani COSMOS: Atmospheric Data', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92003, 0, 'Ani COSMOS: Sealed Archive', 2, 9, 0, 0, 0, 0, 0, 0, 40000, 0, 0, 8000, 0),
(92004, 0, 'Ani COSMOS: Lost Relics', 3, 9, 0, 0, 0, 0, 0, 0, 80000, 0, 0, 16000, 0),
-- Araz constellation (Amarr COSMOS)
(92005, 0, 'Araz COSMOS: Scripture Translation', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92006, 0, 'Araz COSMOS: Imperial Survey', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92007, 0, 'Araz COSMOS: Heretic Cache', 3, 9, 0, 0, 0, 0, 0, 0, 80000, 0, 0, 16000, 0),
(92008, 0, 'Araz COSMOS: Cathedral Records', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Okkelen constellation (Caldari COSMOS)
(92009, 0, 'Okkelen COSMOS: Prototype Salvage', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92010, 0, 'Okkelen COSMOS: Corporate Espionage', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92011, 0, 'Okkelen COSMOS: Encrypted Data Core', 3, 9, 0, 0, 0, 0, 0, 0, 80000, 0, 0, 16000, 0),
-- Algintal constellation (Gallente COSMOS)
(92012, 0, 'Algintal COSMOS: Luxury Goods', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92013, 0, 'Algintal COSMOS: Federal Intelligence', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
(92014, 0, 'Algintal COSMOS: Secret Dispatch', 3, 9, 0, 0, 0, 0, 0, 0, 80000, 0, 0, 16000, 0),
(92015, 0, 'Algintal COSMOS: Presidential Decree', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92000 AND 92015;
