-- W-space Data & Relic sites (EVE University wiki) — containers + Sleeper guards
-- +migrate Up

-- =========================================================================
-- Data sites (dungeonType 3 = Magnetometric) — Unsecured Perimeter/Frontier/Core
-- 2 per class. Containers (typeID 23, groupID 12) guarded by Sleeper Sentinels.
-- =========================================================================
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
-- C1 Data
(4301, 'Unsecured Perimeter Amplifier', 0, 500023, 3, 'wh-c1-d1'),
(4302, 'Unsecured Perimeter Information Center', 0, 500023, 3, 'wh-c1-d2'),
-- C2 Data
(4303, 'Unsecured Perimeter Comms Relay', 0, 500023, 3, 'wh-c2-d1'),
(4304, 'Unsecured Perimeter Transponder Farm', 0, 500023, 3, 'wh-c2-d2'),
-- C3 Data
(4305, 'Unsecured Frontier Database', 0, 500023, 3, 'wh-c3-d1'),
(4306, 'Unsecured Frontier Receiver', 0, 500023, 3, 'wh-c3-d2'),
-- C4 Data
(4307, 'Unsecured Frontier Digital Nexus', 0, 500023, 3, 'wh-c4-d1'),
(4308, 'Unsecured Frontier Trinary Hub', 0, 500023, 3, 'wh-c4-d2'),
-- C5 Data
(4309, 'Unsecured Frontier Enclave Relay', 0, 500023, 3, 'wh-c5-d1'),
(4310, 'Unsecured Frontier Server Bank', 0, 500023, 3, 'wh-c5-d2'),
-- C6 Data
(4311, 'Unsecured Core Backup Array', 0, 500023, 3, 'wh-c6-d1'),
(4312, 'Unsecured Core Emergence', 0, 500023, 3, 'wh-c6-d2');

-- =========================================================================
-- Relic sites (dungeonType 4 = Radar) — Forgotten Perimeter/Frontier/Core
-- 2 per class. Containers guarded by Sleeper Patrollers.
-- =========================================================================
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
-- C1 Relic
(4401, 'Forgotten Perimeter Coronation Platform', 0, 500023, 4, 'wh-c1-r1'),
(4402, 'Forgotten Perimeter Power Array', 0, 500023, 4, 'wh-c1-r2'),
-- C2 Relic
(4403, 'Forgotten Perimeter Gateway', 0, 500023, 4, 'wh-c2-r1'),
(4404, 'Forgotten Perimeter Habitation Coils', 0, 500023, 4, 'wh-c2-r2'),
-- C3 Relic
(4405, 'Forgotten Frontier Quarantine Outpost', 0, 500023, 4, 'wh-c3-r1'),
(4406, 'Forgotten Frontier Recursive Depot', 0, 500023, 4, 'wh-c3-r2'),
-- C4 Relic
(4407, 'Forgotten Frontier Conversion Module', 0, 500023, 4, 'wh-c4-r1'),
(4408, 'Forgotten Frontier Evacuation Center', 0, 500023, 4, 'wh-c4-r2'),
-- C5 Relic
(4409, 'Forgotten Core Data Field', 0, 500023, 4, 'wh-c5-r1'),
(4410, 'Forgotten Core Information Pen', 0, 500023, 4, 'wh-c5-r2'),
-- C6 Relic
(4411, 'Forgotten Core Assembly Hall', 0, 500023, 4, 'wh-c6-r1'),
(4412, 'Forgotten Core Circuitry Disassembler', 0, 500023, 4, 'wh-c6-r2');

-- Rooms: 1 pocket per data/relic site (2 containers + guards)
INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(4301,'Pocket',4301),(4302,'Pocket',4302),(4303,'Pocket',4303),(4304,'Pocket',4304),
(4305,'Pocket',4305),(4306,'Pocket',4306),(4307,'Pocket',4307),(4308,'Pocket',4308),
(4309,'Pocket',4309),(4310,'Pocket',4310),(4311,'Pocket',4311),(4312,'Pocket',4312),
(4401,'Pocket',4401),(4402,'Pocket',4402),(4403,'Pocket',4403),(4404,'Pocket',4404),
(4405,'Pocket',4405),(4406,'Pocket',4406),(4407,'Pocket',4407),(4408,'Pocket',4408),
(4409,'Pocket',4409),(4410,'Pocket',4410),(4411,'Pocket',4411),(4412,'Pocket',4412);

-- Objects: 2 containers (typeID 23) + 2-3 Sleeper guards per pocket.
-- C1/C2 guards = Sleepless Sentinel (30196); C3/C4 = Awakened Sentinel (30206);
-- C5/C6 = Emergent Sentinel (30215).
-- Data sites (4301-4312)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(4301,23,12,0,0,0),(4301,23,12,200,0,150),(4301,30196,959,-300,0,-200),(4301,30196,959,300,0,200),
(4302,23,12,0,0,0),(4302,23,12,-200,0,-150),(4302,30196,959,300,0,200),(4302,30196,959,-300,0,-200),
(4303,23,12,0,0,0),(4303,23,12,200,0,150),(4303,30196,959,-300,0,-200),(4303,30196,959,300,0,200),
(4304,23,12,0,0,0),(4304,23,12,-200,0,-150),(4304,30196,959,300,0,200),(4304,30196,959,-300,0,-200),
(4305,23,12,0,0,0),(4305,23,12,200,0,150),(4305,30206,960,-300,0,-200),(4305,30206,960,300,0,200),
(4306,23,12,0,0,0),(4306,23,12,-200,0,-150),(4306,30206,960,300,0,200),(4306,30206,960,-300,0,-200),
(4307,23,12,0,0,0),(4307,23,12,200,0,150),(4307,30206,960,-300,0,-200),(4307,30206,960,300,0,200),
(4308,23,12,0,0,0),(4308,23,12,-200,0,-150),(4308,30206,960,300,0,200),(4308,30206,960,-300,0,-200),
(4309,23,12,0,0,0),(4309,23,12,200,0,150),(4309,30215,961,-300,0,-200),(4309,30215,961,300,0,200),
(4310,23,12,0,0,0),(4310,23,12,-200,0,-150),(4310,30215,961,300,0,200),(4310,30215,961,-300,0,-200),
(4311,23,12,0,0,0),(4311,23,12,200,0,150),(4311,30215,961,-300,0,-200),(4311,30215,961,300,0,200),
(4312,23,12,0,0,0),(4312,23,12,-200,0,-150),(4312,30215,961,300,0,200),(4312,30215,961,-300,0,-200);

-- Relic sites (4401-4412)
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(4401,23,12,0,0,0),(4401,23,12,200,0,150),(4401,30188,983,-300,0,-200),(4401,30188,983,300,0,200),
(4402,23,12,0,0,0),(4402,23,12,-200,0,-150),(4402,30188,983,300,0,200),(4402,30188,983,-300,0,-200),
(4403,23,12,0,0,0),(4403,23,12,200,0,150),(4403,30188,983,-300,0,-200),(4403,30188,983,300,0,200),
(4404,23,12,0,0,0),(4404,23,12,-200,0,-150),(4404,30188,983,300,0,200),(4404,30188,983,-300,0,-200),
(4405,23,12,0,0,0),(4405,23,12,200,0,150),(4405,30200,985,-300,0,-200),(4405,30200,985,300,0,200),
(4406,23,12,0,0,0),(4406,23,12,-200,0,-150),(4406,30200,985,300,0,200),(4406,30200,985,-300,0,-200),
(4407,23,12,0,0,0),(4407,23,12,200,0,150),(4407,30200,985,-300,0,-200),(4407,30200,985,300,0,200),
(4408,23,12,0,0,0),(4408,23,12,-200,0,-150),(4408,30200,985,300,0,200),(4408,30200,985,-300,0,-200),
(4409,23,12,0,0,0),(4409,23,12,200,0,150),(4409,30209,987,-300,0,-200),(4409,30209,987,300,0,200),
(4410,23,12,0,0,0),(4410,23,12,-200,0,-150),(4410,30209,987,300,0,200),(4410,30209,987,-300,0,-200),
(4411,23,12,0,0,0),(4411,23,12,200,0,150),(4411,30209,987,-300,0,-200),(4411,30209,987,300,0,200),
(4412,23,12,0,0,0),(4412,23,12,-200,0,-150),(4412,30209,987,300,0,200),(4412,30209,987,-300,0,-200);

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 4301 AND 4412;
DELETE FROM `dunRooms` WHERE `roomID` BETWEEN 4301 AND 4412;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 4301 AND 4412;
