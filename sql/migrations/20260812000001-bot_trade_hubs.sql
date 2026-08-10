-- Bot trade hubs — where simulated players buy and sell. Jita is the main
-- market hub; bots haul cargo there to sell, PvP corps buy their fittings there,
-- and couriers run contracts to/from it.
-- +migrate Up
CREATE TABLE IF NOT EXISTS `botTradeHubs` (
    `systemID` INT UNSIGNED NOT NULL,
    `stationID` INT UNSIGNED NOT NULL DEFAULT 0,
    `hubName` VARCHAR(64) NOT NULL DEFAULT '',
    `isPrimary` TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (`systemID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Jita (The Forge) is the primary hub. stationID 60003760 = Jita IV - Moon 4 -
-- Caldari Navy Assembly Plant.
INSERT IGNORE INTO `botTradeHubs` (`systemID`, `stationID`, `hubName`, `isPrimary`)
VALUES (30000142, 60003760, 'Jita', 1);

-- +migrate Down
DROP TABLE IF EXISTS `botTradeHubs`;
