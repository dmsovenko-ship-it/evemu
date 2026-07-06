-- Fix anomaly NPC groupIDs: switch from Entity-category groups to Ship/Drone groups
-- Also set graphicID, radius, soundID to prevent client KeyError and server marshal crash
-- Entity groups (catID=11) don't match the catID == Ship(6) || Drone(18) check in MakeDungeon
-- +migrate Up
UPDATE invTypes SET groupID = 25,  graphicID = 1827, radius = 50,  soundID = 20073 WHERE typeID = 33001; -- Guristas Scout -> Frigate
UPDATE invTypes SET groupID = 26,  graphicID = 1823, radius = 150, soundID = 20073 WHERE typeID = 33002; -- Guristas Raider -> Cruiser
UPDATE invTypes SET groupID = 27,  graphicID = 2158, radius = 350, soundID = 20073 WHERE typeID = 33003; -- Guristas Warlord -> Battleship
UPDATE invTypes SET groupID = 419, graphicID = 1825, radius = 250, soundID = 20073 WHERE typeID = 33004; -- Guristas Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25,  graphicID = 342,  radius = 50,  soundID = 20073 WHERE typeID = 33020; -- Angel Ruffian -> Frigate
UPDATE invTypes SET groupID = 26,  graphicID = 337,  radius = 150, soundID = 20073 WHERE typeID = 33021; -- Angel Marauder -> Cruiser
UPDATE invTypes SET groupID = 27,  graphicID = 335,  radius = 350, soundID = 20073 WHERE typeID = 33022; -- Angel Warlord -> Battleship
UPDATE invTypes SET groupID = 419, graphicID = 336,  radius = 250, soundID = 20073 WHERE typeID = 33023; -- Angel Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25,  graphicID = 1821, radius = 50,  soundID = 20073 WHERE typeID = 33040; -- Serpentis Scout -> Frigate
UPDATE invTypes SET groupID = 26,  graphicID = 1812, radius = 150, soundID = 20073 WHERE typeID = 33041; -- Serpentis Raider -> Cruiser
UPDATE invTypes SET groupID = 27,  graphicID = 2156, radius = 350, soundID = 20073 WHERE typeID = 33042; -- Serpentis Warlord -> Battleship
UPDATE invTypes SET groupID = 419, graphicID = 1813, radius = 250, soundID = 20073 WHERE typeID = 33043; -- Serpentis Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25,  graphicID = 1766, radius = 50,  soundID = 20073 WHERE typeID = 33060; -- Blood Scout -> Frigate
UPDATE invTypes SET groupID = 26,  graphicID = 1760, radius = 150, soundID = 20073 WHERE typeID = 33061; -- Blood Raider -> Cruiser
UPDATE invTypes SET groupID = 27,  graphicID = 2121, radius = 350, soundID = 20073 WHERE typeID = 33062; -- Blood Warlord -> Battleship
UPDATE invTypes SET groupID = 419, graphicID = 1758, radius = 250, soundID = 20073 WHERE typeID = 33063; -- Blood Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25,  graphicID = 1237, radius = 50,  soundID = 20073 WHERE typeID = 33080; -- Sansha Scout -> Frigate
UPDATE invTypes SET groupID = 26,  graphicID = 1236, radius = 150, soundID = 20073 WHERE typeID = 33081; -- Sansha Raider -> Cruiser
UPDATE invTypes SET groupID = 27,  graphicID = 2295, radius = 350, soundID = 20073 WHERE typeID = 33082; -- Sansha Warlord -> Battleship
UPDATE invTypes SET groupID = 419, graphicID = 2289, radius = 250, soundID = 20073 WHERE typeID = 33083; -- Sansha Baron -> Battlecruiser
UPDATE invTypes SET groupID = 100, graphicID = 2078, radius = 50,  soundID = 20073 WHERE typeID = 33100; -- Rogue Drone Scout -> CombatDrone
UPDATE invTypes SET groupID = 100, graphicID = 2078, radius = 150, soundID = 20073 WHERE typeID = 33101; -- Rogue Drone Raider -> CombatDrone
UPDATE invTypes SET groupID = 100, graphicID = 2078, radius = 350, soundID = 20073 WHERE typeID = 33102; -- Rogue Drone Warlord -> CombatDrone
UPDATE invTypes SET groupID = 100, graphicID = 2078, radius = 250, soundID = 20073 WHERE typeID = 33103; -- Rogue Drone Baron -> CombatDrone
-- +migrate Down
UPDATE invTypes SET groupID = 562, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33001;
UPDATE invTypes SET groupID = 561, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33002;
UPDATE invTypes SET groupID = 560, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33003;
UPDATE invTypes SET groupID = 580, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33004;
UPDATE invTypes SET groupID = 550, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33020;
UPDATE invTypes SET groupID = 551, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33021;
UPDATE invTypes SET groupID = 552, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33022;
UPDATE invTypes SET groupID = 576, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33023;
UPDATE invTypes SET groupID = 572, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33040;
UPDATE invTypes SET groupID = 571, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33041;
UPDATE invTypes SET groupID = 570, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33042;
UPDATE invTypes SET groupID = 584, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33043;
UPDATE invTypes SET groupID = 557, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33060;
UPDATE invTypes SET groupID = 555, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33061;
UPDATE invTypes SET groupID = 556, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33062;
UPDATE invTypes SET groupID = 578, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33063;
UPDATE invTypes SET groupID = 567, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33080;
UPDATE invTypes SET groupID = 566, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33081;
UPDATE invTypes SET groupID = 565, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33082;
UPDATE invTypes SET groupID = 582, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33083;
UPDATE invTypes SET groupID = 759, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33100;
UPDATE invTypes SET groupID = 757, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33101;
UPDATE invTypes SET groupID = 756, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33102;
UPDATE invTypes SET groupID = 755, graphicID = NULL, radius = 0,   soundID = NULL WHERE typeID = 33103;
