-- Rogue Drones content: swarm-tier anomalies (Cluster..Horde + new mid/high
-- types), sentry batteries, Sentient commander spawns. All types real SDE.
-- +migrate Up

-- =========================================================================
-- 1. Sentry batteries in existing Rogue Drone anomalies (roomID == dungeonID)
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Drone Cluster (2090) / Collection (2091)
(2090,18023,383,3500,0,2000), (2090,18035,383,-3500,0,2000),
(2091,18035,383,3500,0,2000), (2091,18033,383,-3500,0,-2000),
-- Assembly (2092) / Horde (2093)
(2092,18033,383,3500,0,2000), (2092,18032,383,-3500,0,-2000), (2092,27956,383,0,0,4000),
(2093,18032,383,4500,0,1500), (2093,18031,383,-4500,0,-1500), (2093,27956,383,0,0,4500), (2093,27954,383,-2000,0,4000);

-- =========================================================================
-- 2. New swarm anomalies: Gathering (2140), Surveillance (2141), Menagerie
--    (2142), Herd (2143), Squad (2144), Patrol (2145)
-- =========================================================================
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2140 AND 2145;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2140 AND 2145;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2140 AND 2145;

INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2140, 'Drone Gathering', 0, 500022, 7, 'drone-gathering'),
(2141, 'Drone Surveillance', 0, 500022, 7, 'drone-surveillance'),
(2142, 'Drone Menagerie', 0, 500022, 7, 'drone-menagerie'),
(2143, 'Drone Herd', 0, 500022, 7, 'drone-herd'),
(2144, 'Drone Squad', 0, 500022, 7, 'drone-squad'),
(2145, 'Drone Patrol', 0, 500022, 7, 'drone-patrol');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2140,'Gathering Pocket',2140), (2141,'Surveillance Pocket',2141), (2142,'Menagerie Pocket',2142),
(2143,'Herd Pocket',2143), (2144,'Squad Pocket',2144), (2145,'Patrol Pocket',2145);

INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
-- Gathering: frigate/destroyer/cruiser swarm + Sentient
(2140,3744,805,0,0,1500), (2140,3749,805,-1500,0,0), (2140,3750,805,1500,0,-750),
(2140,23475,804,0,0,-2500), (2140,23476,804,-2500,0,1500),
(2140,3745,803,2500,0,-1500), (2140,3746,803,0,0,3500),
(2140,27738,845,0,0,-4000),                       -- Sentient Annihilator Alvum
(2140,18035,383,3500,0,2000), (2140,18033,383,-3500,0,2000), (2140,27956,383,0,0,-4500),
-- Surveillance: destroyers/cruisers/BCs
(2141,23475,804,0,0,1500), (2141,23476,804,-1500,0,0), (2141,23478,804,1500,0,-750),
(2141,3745,803,0,0,-2500), (2141,3746,803,-2500,0,1500),
(2141,18073,801,2500,0,-1500), (2141,18074,801,0,0,3500),
(2141,18033,383,3500,0,2000), (2141,18032,383,-3500,0,2000), (2141,27956,383,0,0,-4500),
-- Menagerie: cruisers/BCs/battleships
(2142,3745,803,0,0,1500), (2142,3746,803,-1500,0,0), (2142,18078,803,1500,0,-750),
(2142,18073,801,0,0,-2500), (2142,18075,801,-2500,0,1500),
(2142,10299,802,2500,0,-1500), (2142,23501,802,0,0,3500),
(2142,18032,383,3500,0,2000), (2142,18031,383,-3500,0,2000), (2142,27954,383,0,0,-4500),
-- Herd: BCs/battleships + Sentient Patriarch
(2143,18073,801,0,0,1500), (2143,18074,801,-1500,0,0), (2143,18075,801,1500,0,-750),
(2143,10299,802,0,0,-2500), (2143,23501,802,-2500,0,1500), (2143,23503,802,2500,0,-1500),
(2143,27728,844,0,0,-4000),                       -- Sentient Patriarch Alvus
(2143,18032,383,4500,0,2000), (2143,18031,383,-4500,0,2000), (2143,27956,383,0,0,-4500), (2143,27954,383,-2000,0,4000),
-- Squad: mixed + Sentient Dismantler
(2144,3745,803,0,0,1500), (2144,18073,801,-1500,0,0), (2144,18075,801,1500,0,-750),
(2144,10299,802,0,0,-2500), (2144,23501,802,-2500,0,1500), (2144,23504,802,2500,0,-1500),
(2144,27748,846,0,0,-4000),                       -- Sentient Dismantler Alvior
(2144,27738,845,3000,0,3000),                     -- Sentient Annihilator Alvum
(2144,18032,383,4500,0,2000), (2144,18031,383,-4500,0,2000), (2144,27956,383,0,0,-4500), (2144,27955,383,-2000,0,4000),
-- Patrol: battleship swarm + Sentient queens
(2145,10299,802,0,0,1500), (2145,23501,802,-1500,0,0), (2145,23503,802,1500,0,-750),
(2145,23504,802,0,0,-2500), (2145,23500,802,-2500,0,1500),
(2145,27722,844,0,0,-4000),                       -- Sentient Alvus Queen
(2145,27726,844,3000,0,3000),                     -- Sentient Domination Alvus
(2145,27728,844,-3000,0,3000),                    -- Sentient Patriarch Alvus
(2145,18031,383,4500,0,2000), (2145,27955,383,-4500,0,2000), (2145,27956,383,0,0,-4500), (2145,18023,383,-2000,0,4000);

-- =========================================================================
-- 3. DED Rogue Drone 3/10 (2350) — sentry batteries in the three pockets
-- =========================================================================
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2350,18035,383,3000,0,1500), (2350,18023,383,-3000,0,1500),
(2351,18033,383,3000,0,1500), (2351,27956,383,-3000,0,1500),
(2352,18032,383,3000,0,1500), (2352,27954,383,-3000,0,1500);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2140 AND 2145;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 2140 AND 2145;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2140 AND 2145;
