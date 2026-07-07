-- CONCORD LP store for incursion participants
-- CONCORD corporationID = 1000131
-- +migrate Up

-- CONCORD LP Store offers
INSERT IGNORE INTO `lpStore` (`storeID`, `corporationID`, `typeID`, `quantity`, `lpCost`, `iskCost`) VALUES
-- Implants
(101, 1000131, 33520, 1, 30000, 10000000.00),  -- Mid-grade Nomad Alpha
(102, 1000131, 33521, 1, 45000, 15000000.00),  -- Mid-grade Nomad Beta
(103, 1000131, 33522, 1, 60000, 20000000.00),  -- Mid-grade Nomad Gamma
(104, 1000131, 33523, 1, 75000, 25000000.00),  -- Mid-grade Nomad Delta
(105, 1000131, 33524, 1, 90000, 30000000.00),  -- Mid-grade Nomad Epsilon
(106, 1000131, 33525, 1, 105000, 35000000.00), -- Mid-grade Nomad Omega
-- Ship SKINs (CONCORD)
(107, 1000131, 35001, 1, 10000, 5000000.00),   -- CONCORD Battlecruiser SKIN
(108, 1000131, 35002, 1, 15000, 7500000.00),   -- CONCORD Battleship SKIN
-- Implants (low-grade for accessibility)
(109, 1000131, 33510, 1, 15000, 5000000.00),   -- Low-grade Nomad Alpha
(110, 1000131, 33511, 1, 22500, 7500000.00),   -- Low-grade Nomad Beta
(111, 1000131, 33512, 1, 30000, 10000000.00),  -- Low-grade Nomad Gamma
(112, 1000131, 33513, 1, 37500, 12500000.00),  -- Low-grade Nomad Delta
(113, 1000131, 33514, 1, 45000, 15000000.00),  -- Low-grade Nomad Epsilon
(114, 1000131, 33515, 1, 52500, 17500000.00),  -- Low-grade Nomad Omega
-- Nano ribbons (Faction spawn enhancers)
(115, 1000131, 30377, 1, 5000, 1000000.00),    -- Faction Navy Implant
(116, 1000131, 30378, 1, 8000, 2000000.00);    -- Improved Navy Implant

-- +migrate Down
DELETE FROM `lpStore` WHERE `corporationID` = 1000131;
