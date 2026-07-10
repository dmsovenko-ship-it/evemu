-- DED complexes 1/10, 2/10, 4/10, 5/10
-- +migrate Up

-- ====== 1/10 (Frigate) — 1 room, 3-4 frigates + container ======
-- Angel
INSERT IGNORE INTO `dunDungeons` VALUES (2400,'Angel 1/10 DED',3,500011,10,'angel-ded-110');
INSERT IGNORE INTO `dunRooms` VALUES (2400,'Pocket',2400);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2400,23,0,0,0,0),(2400,2372,0,-200,0,150),(2400,2372,0,200,0,-150),(2400,10017,0,0,0,250);
-- Blood
INSERT IGNORE INTO `dunDungeons` VALUES (2410,'Blood 1/10 DED',3,500012,10,'blood-ded-110');
INSERT IGNORE INTO `dunRooms` VALUES (2410,'Pocket',2410);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2410,23,0,0,0,0),(2410,10275,0,-200,0,150),(2410,10275,0,200,0,-150),(2410,10281,0,0,0,250);
-- Guristas
INSERT IGNORE INTO `dunDungeons` VALUES (2420,'Guristas 1/10 DED',3,500010,10,'guristas-ded-110');
INSERT IGNORE INTO `dunRooms` VALUES (2420,'Pocket',2420);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2420,23,0,0,0,0),(2420,2382,0,-200,0,150),(2420,2382,0,200,0,-150),(2420,2387,0,0,0,250);
-- Sansha
INSERT IGNORE INTO `dunDungeons` VALUES (2430,'Sansha 1/10 DED',3,500019,10,'sansha-ded-110');
INSERT IGNORE INTO `dunRooms` VALUES (2430,'Pocket',2430);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2430,23,0,0,0,0),(2430,10025,0,-200,0,150),(2430,10025,0,200,0,-150),(2430,10030,0,0,0,250);
-- Serpentis
INSERT IGNORE INTO `dunDungeons` VALUES (2440,'Serpentis 1/10 DED',3,500013,10,'serpentis-ded-110');
INSERT IGNORE INTO `dunRooms` VALUES (2440,'Pocket',2440);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2440,23,0,0,0,0),(2440,2370,0,-200,0,150),(2440,2370,0,200,0,-150),(2440,2381,0,0,0,250);

-- ====== 2/10 (Destroyer) — 2 rooms, mixed frig/destroyer + containers ======
-- Angel
INSERT IGNORE INTO `dunDungeons` VALUES (2450,'Angel 2/10 DED',3,500011,10,'angel-ded-210');
INSERT IGNORE INTO `dunRooms` VALUES (2450,'Entry',2450),(2451,'Command',2450);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2450,23,0,0,0,0),(2450,2372,0,-250,0,200),(2450,10017,0,250,0,-200),
(2451,23,0,0,0,0),(2451,23,0,150,0,100),(2451,2372,0,-300,0,250),(2451,11898,0,300,0,-250);
-- Blood
INSERT IGNORE INTO `dunDungeons` VALUES (2460,'Blood 2/10 DED',3,500012,10,'blood-ded-210');
INSERT IGNORE INTO `dunRooms` VALUES (2460,'Entry',2460),(2461,'Command',2460);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2460,23,0,0,0,0),(2460,10275,0,-250,0,200),(2460,10281,0,250,0,-200),
(2461,23,0,0,0,0),(2461,23,0,150,0,100),(2461,10275,0,-300,0,250),(2461,11905,0,300,0,-250);
-- Guristas
INSERT IGNORE INTO `dunDungeons` VALUES (2470,'Guristas 2/10 DED',3,500010,10,'guristas-ded-210');
INSERT IGNORE INTO `dunRooms` VALUES (2470,'Entry',2470),(2471,'Command',2470);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2470,23,0,0,0,0),(2470,2382,0,-250,0,200),(2470,2387,0,250,0,-200),
(2471,23,0,0,0,0),(2471,23,0,150,0,100),(2471,2382,0,-300,0,250),(2471,11932,0,300,0,-250);
-- Sansha
INSERT IGNORE INTO `dunDungeons` VALUES (2480,'Sansha 2/10 DED',3,500019,10,'sansha-ded-210');
INSERT IGNORE INTO `dunRooms` VALUES (2480,'Entry',2480),(2481,'Command',2480);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2480,23,0,0,0,0),(2480,10025,0,-250,0,200),(2480,10030,0,250,0,-200),
(2481,23,0,0,0,0),(2481,23,0,150,0,100),(2481,10025,0,-300,0,250),(2481,11913,0,300,0,-250);
-- Serpentis
INSERT IGNORE INTO `dunDungeons` VALUES (2490,'Serpentis 2/10 DED',3,500013,10,'serpentis-ded-210');
INSERT IGNORE INTO `dunRooms` VALUES (2490,'Entry',2490),(2491,'Command',2490);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2490,23,0,0,0,0),(2490,2370,0,-250,0,200),(2490,2381,0,250,0,-200),
(2491,23,0,0,0,0),(2491,23,0,150,0,100),(2491,2370,0,-300,0,250),(2491,11921,0,300,0,-250);

-- ====== 4/10 (Battlecruiser) — 3 rooms, BCs + cruisers + containers ======
-- Angel
INSERT IGNORE INTO `dunDungeons` VALUES (2500,'Angel 4/10 DED',3,500011,10,'angel-ded-410');
INSERT IGNORE INTO `dunRooms` VALUES (2500,'Entry',2500),(2501,'Second',2500),(2502,'Command',2500);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2500,23,0,0,0,0),(2500,2372,0,-300,0,200),(2500,10017,0,300,0,-200),
(2501,23,0,0,0,0),(2501,23,0,-200,0,150),(2501,11898,0,-400,0,300),(2501,10017,0,400,0,-300),
(2502,23,0,0,0,0),(2502,23,0,-200,0,150),(2502,22822,0,-500,0,350),(2502,11898,0,500,0,-350);
-- Blood
INSERT IGNORE INTO `dunDungeons` VALUES (2510,'Blood 4/10 DED',3,500012,10,'blood-ded-410');
INSERT IGNORE INTO `dunRooms` VALUES (2510,'Entry',2510),(2511,'Second',2510),(2512,'Command',2510);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2510,23,0,0,0,0),(2510,10275,0,-300,0,200),(2510,10281,0,300,0,-200),
(2511,23,0,0,0,0),(2511,23,0,-200,0,150),(2511,11905,0,-400,0,300),(2511,10281,0,400,0,-300),
(2512,23,0,0,0,0),(2512,23,0,-200,0,150),(2512,23244,0,-500,0,350),(2512,11905,0,500,0,-350);
-- Guristas
INSERT IGNORE INTO `dunDungeons` VALUES (2520,'Guristas 4/10 DED',3,500010,10,'guristas-ded-410');
INSERT IGNORE INTO `dunRooms` VALUES (2520,'Entry',2520),(2521,'Second',2520),(2522,'Command',2520);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2520,23,0,0,0,0),(2520,2382,0,-300,0,200),(2520,2387,0,300,0,-200),
(2521,23,0,0,0,0),(2521,23,0,-200,0,150),(2521,11932,0,-400,0,300),(2521,2387,0,400,0,-300),
(2522,23,0,0,0,0),(2522,23,0,-200,0,150),(2522,23321,0,-500,0,350),(2522,11932,0,500,0,-350);
-- Sansha
INSERT IGNORE INTO `dunDungeons` VALUES (2530,'Sansha 4/10 DED',3,500019,10,'sansha-ded-410');
INSERT IGNORE INTO `dunRooms` VALUES (2530,'Entry',2530),(2531,'Second',2530),(2532,'Command',2530);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2530,23,0,0,0,0),(2530,10025,0,-300,0,200),(2530,10030,0,300,0,-200),
(2531,23,0,0,0,0),(2531,23,0,-200,0,150),(2531,11913,0,-400,0,300),(2531,10030,0,400,0,-300),
(2532,23,0,0,0,0),(2532,23,0,-200,0,150),(2532,23383,0,-500,0,350),(2532,11913,0,500,0,-350);
-- Serpentis
INSERT IGNORE INTO `dunDungeons` VALUES (2540,'Serpentis 4/10 DED',3,500013,10,'serpentis-ded-410');
INSERT IGNORE INTO `dunRooms` VALUES (2540,'Entry',2540),(2541,'Second',2540),(2542,'Command',2540);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2540,23,0,0,0,0),(2540,2370,0,-300,0,200),(2540,2381,0,300,0,-200),
(2541,23,0,0,0,0),(2541,23,0,-200,0,150),(2541,11921,0,-400,0,300),(2541,2381,0,400,0,-300),
(2542,23,0,0,0,0),(2542,23,0,-200,0,150),(2542,23438,0,-500,0,350),(2542,11921,0,500,0,-350);

-- ====== 5/10 (Battleship) — 4 rooms, BSs + BCs + overseer + containers ======
-- Angel
INSERT IGNORE INTO `dunDungeons` VALUES (2600,'Angel 5/10 DED',3,500011,10,'angel-ded-510');
INSERT IGNORE INTO `dunRooms` VALUES (2600,'Entry',2600),(2601,'Second',2600),(2602,'Third',2600),(2603,'Command',2600);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2600,23,0,0,0,0),(2600,2372,0,-350,0,250),(2600,10017,0,350,0,-250),
(2601,23,0,0,0,0),(2601,23,0,-250,0,200),(2601,11898,0,-500,0,400),(2601,22822,0,500,0,-400),
(2602,23,0,0,0,0),(2602,23,0,-250,0,200),(2602,11898,0,-600,0,450),(2602,22822,0,600,0,-450),
(2603,23,0,0,0,0),(2603,23,0,-250,0,200),(2603,23,0,250,0,-200),(2603,22822,0,-700,0,500),(2603,11898,0,700,0,-500);
-- Blood
INSERT IGNORE INTO `dunDungeons` VALUES (2610,'Blood 5/10 DED',3,500012,10,'blood-ded-510');
INSERT IGNORE INTO `dunRooms` VALUES (2610,'Entry',2610),(2611,'Second',2610),(2612,'Third',2610),(2613,'Command',2610);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2610,23,0,0,0,0),(2610,10275,0,-350,0,250),(2610,10281,0,350,0,-250),
(2611,23,0,0,0,0),(2611,23,0,-250,0,200),(2611,11905,0,-500,0,400),(2611,23244,0,500,0,-400),
(2612,23,0,0,0,0),(2612,23,0,-250,0,200),(2612,11905,0,-600,0,450),(2612,23244,0,600,0,-450),
(2613,23,0,0,0,0),(2613,23,0,-250,0,200),(2613,23,0,250,0,-200),(2613,23244,0,-700,0,500),(2613,11905,0,700,0,-500);
-- Guristas
INSERT IGNORE INTO `dunDungeons` VALUES (2620,'Guristas 5/10 DED',3,500010,10,'guristas-ded-510');
INSERT IGNORE INTO `dunRooms` VALUES (2620,'Entry',2620),(2621,'Second',2620),(2622,'Third',2620),(2623,'Command',2620);
INSERT IGNORE INTO `dunRoomObjects` (`roomID`,`typeID`,`groupID`,`x`,`y`,`z`) VALUES
(2620,23,0,0,0,0),(2620,2382,0,-350,0,250),(2620,2387,0,350,0,-250),
(2621,23,0,0,0,0),(2621,23,0,-250,0,200),(2621,11932,0,-500,0,400),(2621,23321,0,500,0,-400),
(2622,23,0,0,0,0),(2622,23,0,-250,0,200),(2622,11932,0,-600,0,450),(2622,23321,0,600,0,-450),
(2623,23,0,0,0,0),(2623,23,0,-250,0,200),(2623,23,0,250,0,-200),(2623,23321,0,-700,0,500),(2623,11932,0,700,0,-500);

-- +migrate Down
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2400 AND 2629;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2400 AND 2629;
