-- crpRoles: character → corp role bitmask mapping for mail filtering
CREATE TABLE IF NOT EXISTS `crpRoles` (
    `characterID` INT UNSIGNED NOT NULL,
    `roleID` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`characterID`)
) ENGINE=InnoDB;

-- Seed from entity table (rolesAtAll column) where available
INSERT IGNORE INTO crpRoles (characterID, roleID)
SELECT itemID, customInfo FROM entity
WHERE categoryID = 1 AND customInfo IS NOT NULL AND customInfo != ''
ON DUPLICATE KEY UPDATE roleID = VALUES(roleID);
