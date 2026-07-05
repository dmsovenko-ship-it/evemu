-- COSMOS: Ani constellation + one-time completion tracking
-- +migrate Up

-- COSMOS completion tracking (one-time only)
CREATE TABLE IF NOT EXISTS `chrCosmosState` (
  `characterID` int(10) NOT NULL DEFAULT 0,
  `missionID` int(10) NOT NULL DEFAULT 0,
  `agentID` int(10) NOT NULL DEFAULT 0,
  `completed` tinyint(1) NOT NULL DEFAULT 0,
  `dateCompleted` bigint(20) NOT NULL DEFAULT 0,
  PRIMARY KEY (`characterID`, `missionID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Ani COSMOS agents (agentTypeID=12 = Cosmos)
-- These are special NPCs that offer COSMOS missions in the Ani constellation
-- stationIDs are existing stations in each system
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
-- Hjoramold
(3019515, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Abotur Kverkinn - Lord Bastion
(3019516, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Ekdit Spitek - Machine Head
(3019517, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Nabur Verkort - Machine Head
(3019518, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Sungur Tyrfin - Lord Bastion
(3019519, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Sydri Namian - Lord Bastion
-- Barkrik
(3019520, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Bukar Robaerger - Sister Camp
(3019521, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Dagras Kutill - The Carnival
(3019522, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Mazed Karadom - The Carnival
(3019523, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Mwaku Ristiger - The Hyperbole Nexus
(3019524, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Jippon Frain - The Hyperbole Nexus
(3019525, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Akraun Maertigor - The Hyperbole Nexus
-- Uriok
(3019526, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Aradin Ucham - Assassin's Overhang
(3019527, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Ramakell Tikrest - Culture Recess
(3019528, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Schebach Korten - Insurgent Encampment
(3019529, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Tarak Horkund - Insurgent Encampment
(3019530, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Nafrid Sharum - Assassin's Overhang
-- Traun
(3019531, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Krakmir Mork - Thin Red Line
(3019532, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Krak Hakkars - Thin Red Line
(3019533, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Poreg Murchor - Thin Red Line
(3019534, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Beduim Quereg - Reclamation Wreck
(3019535, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Damos Ossiam - Reclamation Wreck
-- Lanngisi
(3019536, 24, 1000182, 60015093, 1, 20, 12, 0),  -- Beris Nitrus - Sanctum Psychosis
(3019537, 24, 1000182, 60015093, 1, 20, 12, 0),  -- Fara Bokh - Sanctum Psychosis
(3019538, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Godun Sakt - The Asylum
-- Nakugard
(3019539, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Dalkar Kersos - The Glass Edge
(3019540, 24, 1000182, 60015093, 1, 20, 12, 0),  -- Kraimir Mork - The Glass Edge
(3019541, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Penda Rakken - The Glass Edge
(3019542, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Them Burkur - The Glass Edge
(3019543, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Mitsu Hekken - Reactor Factory
-- Inder
(3019544, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Mattheu Rochet - Dream Port
(3019545, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Sinogor Nitrut - Dream Port
(3019546, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Vlas Takson - Dream Port
(3019547, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Nina Darrchien - Rich Man's Run
-- Tvink
(3019548, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Hinrich Tekrawhol - Crystal Dust Compound
(3019549, 24, 1000182, 60015093, 4, 20, 12, 0),  -- Misnik Sarbaert - Crystal Dust Compound
(3019550, 24, 1000182, 60015093, 2, 20, 12, 0),  -- Temer Rugaert - Margin of Error
(3019551, 24, 1000182, 60015093, 6, 20, 12, 0),  -- Nassor Tromkurt - Crystal Dust Compound
-- Barkrik gate agents (high standing BPC rewards)
(3019552, 24, 1000182, 60015093, 9, 20, 12, 0),  -- Makor Desto - Hjoramold gate
(3019553, 24, 1000182, 60015093, 8, 20, 12, 0),  -- Mutama Czeik - Hjoramold gate
(3019554, 24, 1000182, 60015093, 9, 20, 12, 0);  -- Thora Desto - Hjoramold gate

-- COSMOS-specific mission entries
INSERT IGNORE INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
-- Abotur Kverkinn (L2)
(92100, 0, 'Ani C: Sleeper Nanite Cluster', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
-- Ekdit Spitek (L2)
(92101, 0, 'Ani C: Stolen Research Data', 2, 9, 0, 0, 0, 0, 0, 0, 35000, 0, 0, 7000, 0),
-- Nabur Verkort (L4) 
(92102, 0, 'Ani C: Sispur''s Security Logs', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Aradin Ucham (L4)
(92103, 0, 'Ani C: Spy ID Slice', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Ramakell Tikrest (L4)
(92104, 0, 'Ani C: Navy Issue Amplifier', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Bukar Robaerger (L4)
(92105, 0, 'Ani C: Okham''s Head', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Dagras Kutill (L4)
(92106, 0, 'Ani C: Carnival Cipher', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Krakmir Mork (L4)
(92107, 0, 'Ani C: Thin Red Line Intel', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Beris Nitrus (L1)
(92108, 0, 'Ani C: Sanctum Investigation', 1, 9, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
-- Godun Sakt (L4)
(92109, 0, 'Ani C: Asylum Secrets', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Hinrich Tekrawhol (L4)
(92110, 0, 'Ani C: Narcotic Evidence', 4, 9, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
-- Makor Desto (L9 - faction BPC)
(92111, 0, 'Ani C: Angel Diamond Hunt', 5, 9, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0);

-- +migrate Down
DELETE FROM `chrCosmosState`;
DELETE FROM `agtMissions` WHERE `id` BETWEEN 92100 AND 92111;
DELETE FROM `agtAgents` WHERE `agentID` BETWEEN 3019515 AND 3019554;
