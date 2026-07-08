-- Notification tables for the EVE mail notification system
CREATE TABLE IF NOT EXISTS `notification` (
    `notificationID` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `typeID` INT UNSIGNED NOT NULL DEFAULT 0,
    `senderID` INT UNSIGNED NOT NULL DEFAULT 0,
    `receiverID` INT UNSIGNED NOT NULL DEFAULT 0,
    `processed` TINYINT(1) NOT NULL DEFAULT 0,
    `created` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `deleted` TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (`notificationID`),
    KEY `receiverID` (`receiverID`),
    KEY `receiverID_processed` (`receiverID`, `processed`)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `notificationText` (
    `notificationID` BIGINT UNSIGNED NOT NULL,
    `data` TEXT NOT NULL,
    PRIMARY KEY (`notificationID`)
) ENGINE=InnoDB;
