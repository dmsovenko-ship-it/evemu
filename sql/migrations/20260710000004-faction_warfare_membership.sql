-- +migrate Up
CREATE TABLE IF NOT EXISTS `facWarCharacters` (
    `characterID` INT UNSIGNED NOT NULL PRIMARY KEY,
    `factionID` INT UNSIGNED NOT NULL,
    `joined` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `lastActive` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `currentRank` INT UNSIGNED NOT NULL DEFAULT 0,
    `highestRank` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `facWarStats` (
    `characterID` INT UNSIGNED NOT NULL,
    `factionID` INT UNSIGNED NOT NULL,
    `kills` INT UNSIGNED NOT NULL DEFAULT 0,
    `losses` INT UNSIGNED NOT NULL DEFAULT 0,
    `victoryPoints` DOUBLE NOT NULL DEFAULT 0,
    PRIMARY KEY (`characterID`, `factionID`)
) ENGINE=InnoDB;

-- +migrate Down
DROP TABLE IF EXISTS `facWarStats`;
DROP TABLE IF EXISTS `facWarCharacters`;
