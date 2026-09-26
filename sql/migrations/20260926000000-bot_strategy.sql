-- +migrate Up
-- Persistent "brain" memory: learned strategies per (profession, threat kind).
-- Complex situations (you died to NPCs/chelos/humans) are analysed ONCE by the
-- DeepSeek brain; the resulting directive (GUARDS,DRONES,FLEE,FLEET,AVOID,REFIT)
-- is stored here and reused, so we do not ask the LLM every time.
CREATE TABLE IF NOT EXISTS `botStrategy` (
  `strategyKey` VARCHAR(64)  NOT NULL,
  `profession`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `threat`      VARCHAR(24)  NOT NULL DEFAULT '',
  `advice`      VARCHAR(255) NOT NULL DEFAULT '',
  `uses`        INT UNSIGNED NOT NULL DEFAULT 0,
  `lastUse`     DATETIME     NULL,
  `created`     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`strategyKey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botStrategy`;
