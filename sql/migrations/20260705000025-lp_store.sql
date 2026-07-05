-- LP Store tables and offers for empire corps + 4 FW militias
-- lpStore: corporationID → typeID, iskCost, quantity, lpCost
-- lpRequiredItems: parentID → typeID, quantity (items player must provide)
-- +migrate Up

CREATE TABLE IF NOT EXISTS `lpStore` (
  `storeID` int(11) NOT NULL AUTO_INCREMENT,
  `corporationID` int(11) NOT NULL DEFAULT 0,
  `typeID` int(11) NOT NULL DEFAULT 0,
  `quantity` int(11) NOT NULL DEFAULT 1,
  `lpCost` int(11) NOT NULL DEFAULT 0,
  `iskCost` decimal(17,2) NOT NULL DEFAULT 0.00,
  PRIMARY KEY (`storeID`),
  KEY `corporationID` (`corporationID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `lpRequiredItems` (
  `parentID` int(11) NOT NULL DEFAULT 0,
  `typeID` int(11) NOT NULL DEFAULT 0,
  `quantity` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`parentID`,`typeID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- 24th Imperial Crusade (1000179) — Amarr Navy LP store
INSERT IGNORE INTO `lpStore` (`storeID`, `corporationID`, `typeID`, `quantity`, `lpCost`, `iskCost`) VALUES
(1, 1000179, 17709, 1, 35000, 8500000.00),   -- Maller Navy Issue
(2, 1000179, 17713, 1, 40000, 12000000.00),   -- Omen Navy Issue
(3, 1000179, 3516,  1, 60000, 25000000.00),   -- Harbinger Navy Issue
(4, 1000179, 24698, 1, 100000, 95000000.00),  -- Apocalypse Navy Issue
(5, 1000179, 17715, 1, 40000, 10000000.00),   -- Augoror Navy Issue
(6, 1000179, 17711, 1, 40000, 11000000.00),   -- Arbitrator Navy Issue
(7, 1000179, 12753, 1, 15000, 5000000.00),    -- Imperial Navy Medium Armor Repairer
(8, 1000179, 12763, 1, 12000, 3500000.00),    -- Imperial Navy Heat Sink
(9, 1000179, 12814, 1, 10000, 2000000.00),    -- Imperial Navy Energized Adaptive Nano Membrane
(10, 1000179, 31860, 1, 8000, 1500000.00);    -- Imperial Navy Multispectrum Coating

-- State Protectorate (1000180) — Caldari Navy LP store
INSERT IGNORE INTO `lpStore` (`storeID`, `corporationID`, `typeID`, `quantity`, `lpCost`, `iskCost`) VALUES
(11, 1000180, 17619, 1, 8000, 1500000.00),    -- Hookbill
(12, 1000180, 17634, 1, 40000, 12000000.00),  -- Caracal Navy Issue
(13, 1000180, 17636, 1, 60000, 25000000.00),  -- Ferox Navy Issue
(14, 1000180, 24696, 1, 100000, 95000000.00), -- Scorpion Navy Issue
(15, 1000180, 17633, 1, 40000, 10000000.00),  -- Osprey Navy Issue
(16, 1000180, 17635, 1, 40000, 11000000.00),  -- Blackbird Navy Issue
(17, 1000180, 12751, 1, 15000, 5000000.00),   -- Caldari Navy Medium Shield Booster
(18, 1000180, 12769, 1, 12000, 3500000.00),   -- Caldari Navy Ballistic Control System
(19, 1000180, 12808, 1, 10000, 2000000.00),   -- Caldari Navy Invulnerability Field
(20, 1000180, 31854, 1, 8000, 1500000.00);    -- Caldari Navy Multispectrum Shield Hardener

-- Federal Defence Union (1000181) — Gallente Navy LP store
INSERT IGNORE INTO `lpStore` (`storeID`, `corporationID`, `typeID`, `quantity`, `lpCost`, `iskCost`) VALUES
(21, 1000181, 17612, 1, 8000, 1500000.00),    -- Tristan Navy Issue
(22, 1000181, 17614, 1, 40000, 12000000.00),  -- Thorax Navy Issue
(23, 1000181, 17616, 1, 60000, 25000000.00),  -- Brutix Navy Issue
(24, 1000181, 24692, 1, 100000, 95000000.00), -- Dominix Navy Issue
(25, 1000181, 17615, 1, 40000, 10000000.00),  -- Exequror Navy Issue
(26, 1000181, 17613, 1, 40000, 11000000.00),  -- Vexor Navy Issue
(27, 1000181, 12755, 1, 15000, 5000000.00),   -- Federation Navy Medium Armor Repairer
(28, 1000181, 12759, 1, 12000, 3500000.00),   -- Federation Navy Magnetic Field Stabilizer
(29, 1000181, 12810, 1, 10000, 2000000.00),   -- Federation Navy Energized Adaptive Nano Membrane
(30, 1000181, 31862, 1, 8000, 1500000.00);    -- Federation Navy Multispectrum Coating

-- Tribal Liberation Force (1000182) — Minmatar Navy LP store
INSERT IGNORE INTO `lpStore` (`storeID`, `corporationID`, `typeID`, `quantity`, `lpCost`, `iskCost`) VALUES
(31, 1000182, 17700, 1, 8000, 1500000.00),    -- Rifter Fleet Issue
(32, 1000182, 17701, 1, 40000, 12000000.00),  -- Scythe Fleet Issue
(33, 1000182, 17620, 1, 60000, 25000000.00),  -- Hurricane Fleet Issue
(34, 1000182, 24694, 1, 100000, 95000000.00), -- Tempest Fleet Issue
(35, 1000182, 17703, 1, 40000, 10000000.00),  -- Stabber Fleet Issue
(36, 1000182, 17704, 1, 40000, 11000000.00),  -- Rupture Fleet Issue
(37, 1000182, 12757, 1, 15000, 5000000.00),   -- Republic Fleet Medium Shield Booster
(38, 1000182, 12761, 1, 12000, 3500000.00),   -- Republic Fleet Gyrostabilizer
(39, 1000182, 12812, 1, 10000, 2000000.00),   -- Republic Fleet Adaptive Nano Plating
(40, 1000182, 31856, 1, 8000, 1500000.00);    -- Republic Fleet Multispectrum Shield Hardener

-- +migrate Down
DROP TABLE IF EXISTS `lpRequiredItems`;
DROP TABLE IF EXISTS `lpStore`;
