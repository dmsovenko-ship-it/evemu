-- Bot legends from real EVE killmails (zKillboard).
-- Each row = one real player pilot captured from a killmail: their identity,
-- the ship hull they flew, and their fitted modules. BotMgr uses this to build
-- believable player legends (names, corps, ships, fit, skill tier).
-- +migrate Up
CREATE TABLE IF NOT EXISTS `botKillmailLegends` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `killmail_id` BIGINT UNSIGNED NOT NULL,
    `character_id` INT UNSIGNED NOT NULL,
    `character_name` VARCHAR(128) NOT NULL DEFAULT '',
    `corporation_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `alliance_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `ship_type_id` INT UNSIGNED NOT NULL DEFAULT 0,
    `fitted_item_ids` TEXT NULL,          -- JSON array of module typeIDs from the fit
    `security_status` FLOAT NOT NULL DEFAULT 0,
    `kill_time` DATETIME NOT NULL,
    `points` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uqm_killmail_char` (`killmail_id`, `character_id`),
    KEY `idx_character` (`character_id`),
    KEY `idx_ship` (`ship_type_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botKillmailLegends`;
