-- +migrate Up
-- Persistent officer-hunt escalation: how many ships a corporation brings when
-- hunting a strong officer NPC. Grows on losses, resets on a kill, survives
-- server restarts (so the AI brain's lesson is not lost).
CREATE TABLE IF NOT EXISTS `corpOfficerFleet` (
  `corpID`    INT UNSIGNED    NOT NULL,
  `fleetSize` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `updated`   DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`corpID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `corpOfficerFleet`;
