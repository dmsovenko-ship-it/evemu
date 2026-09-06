-- Sovereignty / influence change journal ("смена влияния"). Records every time a
-- system's controlling owner changes so the portal & Telegram can surface it:
--   * FW system flips  -> ownerType 'faction'  (facWarSystems.occupierID changes)
--   * Sov claim/release -> ownerType 'alliance' (mapSystemSovInfo.allianceID added/removed)
-- oldOwnerID/newOwnerID = 0 means "none/unclaimed". changeTime is a Windows filetime.
-- +migrate Up
CREATE TABLE IF NOT EXISTS `sovChangeLog` (
    `changeID` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `systemID` INT UNSIGNED NOT NULL,
    `ownerType` VARCHAR(10) NOT NULL DEFAULT 'faction',
    `oldOwnerID` INT UNSIGNED NOT NULL DEFAULT 0,
    `newOwnerID` INT UNSIGNED NOT NULL DEFAULT 0,
    `changeTime` BIGINT NOT NULL DEFAULT 0,
    PRIMARY KEY (`changeID`),
    KEY `idx_time` (`changeTime`),
    KEY `idx_system` (`systemID`)
) ENGINE=InnoDB;

-- +migrate Down
DROP TABLE IF EXISTS `sovChangeLog`;
