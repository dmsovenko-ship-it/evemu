-- Fix anomaly NPC groupIDs: switch from Entity-category groups to Ship/Drone groups
-- Entity groups (catID=11) don't match the catID == Ship(6) || Drone(18) check in MakeDungeon
-- +migrate Up
UPDATE invTypes SET groupID = 25  WHERE typeID = 33001; -- Guristas Scout -> Frigate
UPDATE invTypes SET groupID = 26  WHERE typeID = 33002; -- Guristas Raider -> Cruiser
UPDATE invTypes SET groupID = 27  WHERE typeID = 33003; -- Guristas Warlord -> Battleship
UPDATE invTypes SET groupID = 419 WHERE typeID = 33004; -- Guristas Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25  WHERE typeID = 33020; -- Angel Ruffian -> Frigate
UPDATE invTypes SET groupID = 26  WHERE typeID = 33021; -- Angel Marauder -> Cruiser
UPDATE invTypes SET groupID = 27  WHERE typeID = 33022; -- Angel Warlord -> Battleship
UPDATE invTypes SET groupID = 419 WHERE typeID = 33023; -- Angel Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25  WHERE typeID = 33040; -- Serpentis Scout -> Frigate
UPDATE invTypes SET groupID = 26  WHERE typeID = 33041; -- Serpentis Raider -> Cruiser
UPDATE invTypes SET groupID = 27  WHERE typeID = 33042; -- Serpentis Warlord -> Battleship
UPDATE invTypes SET groupID = 419 WHERE typeID = 33043; -- Serpentis Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25  WHERE typeID = 33060; -- Blood Scout -> Frigate
UPDATE invTypes SET groupID = 26  WHERE typeID = 33061; -- Blood Raider -> Cruiser
UPDATE invTypes SET groupID = 27  WHERE typeID = 33062; -- Blood Warlord -> Battleship
UPDATE invTypes SET groupID = 419 WHERE typeID = 33063; -- Blood Baron -> Battlecruiser
UPDATE invTypes SET groupID = 25  WHERE typeID = 33080; -- Sansha Scout -> Frigate
UPDATE invTypes SET groupID = 26  WHERE typeID = 33081; -- Sansha Raider -> Cruiser
UPDATE invTypes SET groupID = 27  WHERE typeID = 33082; -- Sansha Warlord -> Battleship
UPDATE invTypes SET groupID = 419 WHERE typeID = 33083; -- Sansha Baron -> Battlecruiser
UPDATE invTypes SET groupID = 100 WHERE typeID = 33100; -- Rogue Drone Scout -> CombatDrone
UPDATE invTypes SET groupID = 100 WHERE typeID = 33101; -- Rogue Drone Raider -> CombatDrone
UPDATE invTypes SET groupID = 100 WHERE typeID = 33102; -- Rogue Drone Warlord -> CombatDrone
UPDATE invTypes SET groupID = 100 WHERE typeID = 33103; -- Rogue Drone Baron -> CombatDrone
-- +migrate Down
UPDATE invTypes SET groupID = 562 WHERE typeID = 33001;
UPDATE invTypes SET groupID = 561 WHERE typeID = 33002;
UPDATE invTypes SET groupID = 560 WHERE typeID = 33003;
UPDATE invTypes SET groupID = 580 WHERE typeID = 33004;
UPDATE invTypes SET groupID = 550 WHERE typeID = 33020;
UPDATE invTypes SET groupID = 551 WHERE typeID = 33021;
UPDATE invTypes SET groupID = 552 WHERE typeID = 33022;
UPDATE invTypes SET groupID = 576 WHERE typeID = 33023;
UPDATE invTypes SET groupID = 572 WHERE typeID = 33040;
UPDATE invTypes SET groupID = 571 WHERE typeID = 33041;
UPDATE invTypes SET groupID = 570 WHERE typeID = 33042;
UPDATE invTypes SET groupID = 584 WHERE typeID = 33043;
UPDATE invTypes SET groupID = 557 WHERE typeID = 33060;
UPDATE invTypes SET groupID = 555 WHERE typeID = 33061;
UPDATE invTypes SET groupID = 556 WHERE typeID = 33062;
UPDATE invTypes SET groupID = 578 WHERE typeID = 33063;
UPDATE invTypes SET groupID = 567 WHERE typeID = 33080;
UPDATE invTypes SET groupID = 566 WHERE typeID = 33081;
UPDATE invTypes SET groupID = 565 WHERE typeID = 33082;
UPDATE invTypes SET groupID = 582 WHERE typeID = 33083;
UPDATE invTypes SET groupID = 759 WHERE typeID = 33100;
UPDATE invTypes SET groupID = 757 WHERE typeID = 33101;
UPDATE invTypes SET groupID = 756 WHERE typeID = 33102;
UPDATE invTypes SET groupID = 755 WHERE typeID = 33103;
