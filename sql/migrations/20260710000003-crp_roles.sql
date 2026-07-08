-- +migrate Up
DROP TABLE IF EXISTS `crpRoles`;
CREATE TABLE `crpRoles` (
    `characterID` INT UNSIGNED NOT NULL,
    `roleID` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`characterID`)
) ENGINE=InnoDB;

-- (roles populated by server at runtime when characters log in)

-- +migrate Down
DROP TABLE IF EXISTS `crpRoles`;
