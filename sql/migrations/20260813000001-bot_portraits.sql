-- Maps bot server character ids to the real EVE killmail character they came
-- from, so portraits can be fetched from ESI by the real id and stored under the
-- server id in image_cache (image_server reads Character/{id}_512.jpg).
-- +migrate Up
CREATE TABLE IF NOT EXISTS `botPortraits` (
    `serverCharID` INT UNSIGNED NOT NULL,
    `eveCharID` INT UNSIGNED NOT NULL,
    `fetched` TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (`serverCharID`),
    KEY `idx_eve` (`eveCharID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botPortraits`;
