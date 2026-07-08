-- +migrate Up
CREATE TABLE IF NOT EXISTS `crpRoles` (
    `characterID` INT UNSIGNED NOT NULL,
    `roleID` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`characterID`)
) ENGINE=InnoDB;

-- Seed from chrCharacters corporationID as placeholder (roles populated by server at runtime)
INSERT IGNORE INTO crpRoles (characterID, roleID)
SELECT characterID, 0 FROM chrCharacters;

-- +migrate Down
DROP TABLE IF EXISTS `crpRoles`;
