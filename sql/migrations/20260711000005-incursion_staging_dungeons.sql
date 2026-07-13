-- Incursion Staging site dungeons + reward group
-- +migrate Up

-- Staging sites (dungeonID 2130-2139)
INSERT IGNORE INTO `dunDungeons` (`dungeonID`, `dungeonName`, `dungeonStatus`, `factionID`, `archetypeID`, `dungeonUUID`) VALUES
(2130, 'Incursion Staging - Distress Beacon', 3, 500019, 7, 'inc-st-distress'),
(2131, 'Incursion Staging - Forward Recon Outpost', 3, 500019, 7, 'inc-st-recon'),
(2132, 'Incursion Staging - Nation Industrial Proxy', 3, 500019, 7, 'inc-st-industrial'),
(2133, 'Incursion Staging - Propaganda Cluster', 3, 500019, 7, 'inc-st-propaganda');

INSERT IGNORE INTO `dunRooms` (`roomID`, `roomName`, `dungeonID`) VALUES
(2130, 'ST Pocket', 2130), (2131, 'ST Pocket', 2131),
(2132, 'ST Pocket', 2132), (2133, 'ST Pocket', 2133);

-- Staging spawns: light frigates + few cruisers
INSERT IGNORE INTO `dunRoomObjects` (`roomID`, `typeID`, `groupID`, `x`, `y`, `z`) VALUES
(2130,10025,0,0,0,1500),(2130,10025,0,1500,0,-750),(2130,10030,0,-1500,0,0),
(2131,10025,0,0,0,1500),(2131,10030,0,1500,0,-750),(2131,10025,0,-1500,0,0),(2131,10030,0,0,0,-3000),
(2132,10025,0,0,0,1500),(2132,10025,0,1500,0,-750),(2132,10025,0,-1500,0,0),(2132,10030,0,0,0,-3000),
(2133,10030,0,0,0,1500),(2133,10025,0,1500,0,-750),(2133,10030,0,-1500,0,0),(2133,10025,0,0,0,-3000);

-- Staging reward: 3.5M + 400 LP (rewardGroup 5)
INSERT IGNORE INTO incursionRewards (rewardGroupID, rewardTypeID, rewardQuantity, lpTypeID, lpAmount) VALUES
(5, 0, 3500000, 0, 400);

-- Update SpawnSites in IncursionMgr to include staging
-- Staging uses dungeonID 2130-2133, sceneType=4

-- +migrate Down
DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2130 AND 2133;
DELETE FROM `dunRooms` WHERE `dungeonID` BETWEEN 2130 AND 2133;
DELETE FROM `dunDungeons` WHERE `dungeonID` BETWEEN 2130 AND 2133;
DELETE FROM incursionRewards WHERE rewardGroupID = 5;
