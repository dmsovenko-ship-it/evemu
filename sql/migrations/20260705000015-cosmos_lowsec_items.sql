-- COSMOS: lowsec/nullsec agents + unique mission items
-- +migrate Up

-- Lowsec COSMOS: Tartatven constellation agents
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019570, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Tagrina Angi
(3019571, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Aville Ancare
(3019572, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Daemire Adamia
(3019573, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Esordik Mitt
(3019574, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Ison Tiadala
(3019575, 24, 1000182, 60015093, 6, 20, 12, 0);  -- Wirdar Erazako

-- Nullsec COSMOS: 760-9C constellation agents
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019576, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Akelf Ortar
(3019577, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Androver Hnill
(3019578, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Ilkur Eiren
(3019579, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Sigulo Ansa
(3019580, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Lafuni Oduntra
(3019581, 24, 1000182, 60015093, 1, 20, 12, 0),  -- Riluko Hik
(3019582, 24, 1000182, 60015093, 1, 20, 12, 0),  -- Uiswin Aurtur
(3019583, 24, 1000182, 60015093, 1, 20, 12, 0);  -- Zwod Aden

-- COSMOS missions for lowsec/nullsec
INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(92140, 0, 'Tartatven C: Alliance Munitions', 4, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92141, 0, 'Tartatven C: Mercenary Intel', 4, 9, 0, 0, 0, 0, 0, 0, 250000, 0, 0, 50000, 0),
(92142, 0, 'Tartatven C: Faction Documents', 6, 9, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(92143, 0, 'Nullsec C: Deep Space Survey', 6, 9, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(92144, 0, 'Nullsec C: Pirate Negotiations', 6, 9, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(92145, 0, 'Nullsec C: Forgotten Outpost', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(92146, 0, 'Nullsec C: Wreckage Scavenging', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0);

-- +migrate Down
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92140 AND 92146;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019570 AND 3019583;
