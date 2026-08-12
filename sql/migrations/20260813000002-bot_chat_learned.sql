-- Learned chat phrases for simulated players (bots).
-- When a bot says a line and someone (player or bot) replies, the bot records
-- the (trigger, reply) pair. Later, when a similar line appears in local, the
-- bot answers from its learned phrases instead of (or before) calling DeepSeek —
-- so bot conversation is mostly built from remembered exchanges, not an API.
-- +migrate Up
CREATE TABLE IF NOT EXISTS `botChatLearned` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `charID` INT UNSIGNED NOT NULL,              -- bot that learned the phrase
    `trigger` VARCHAR(255) NOT NULL,             -- the line it said / was addressed with
    `reply` VARCHAR(255) NOT NULL,               -- the reply it got back
    `uses` INT UNSIGNED NOT NULL DEFAULT 1,      -- how often this reply was reused
    `lastUse` DATETIME NOT NULL,
    PRIMARY KEY (`id`),
    KEY `charID` (`charID`),
    KEY `trigger` (`trigger`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botChatLearned`;
