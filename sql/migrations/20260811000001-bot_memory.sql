-- Self-learning memory for simulated players (bots).
-- Each bot accumulates experience over its lifetime: combat outcomes, chat
-- behaviour, and profession activity (ratting, mining, trading, hacking).
-- BotMgr uses this to make bots act smarter over time — aggressive after wins,
-- cautious after losses; better at their job with practice; reusing chat lines
-- that got responses.
-- +migrate Up
CREATE TABLE IF NOT EXISTS `botMemory` (
    `charID` INT UNSIGNED NOT NULL,
    `wins` INT UNSIGNED NOT NULL DEFAULT 0,        -- fights won (enemy destroyed/fled)
    `losses` INT UNSIGNED NOT NULL DEFAULT 0,      -- fights lost (we fled or died)
    `kills` INT UNSIGNED NOT NULL DEFAULT 0,       -- enemy ships destroyed
    `deaths` INT UNSIGNED NOT NULL DEFAULT 0,      -- times we died
    `chatLines` INT UNSIGNED NOT NULL DEFAULT 0,   -- chat lines sent
    `chatReplies` INT UNSIGNED NOT NULL DEFAULT 0, -- times others replied to us
    `ratKills` INT UNSIGNED NOT NULL DEFAULT 0,    -- NPC rats destroyed (PvE ratting)
    `mineRuns` INT UNSIGNED NOT NULL DEFAULT 0,    -- mining sessions completed
    `tradeRuns` INT UNSIGNED NOT NULL DEFAULT 0,   -- trade/courier runs completed
    `hackRuns` INT UNSIGNED NOT NULL DEFAULT 0,    -- data/relic hacks completed
    `pvpMistakes` INT UNSIGNED NOT NULL DEFAULT 0, -- PvP fights misjudged (attacked & lost / fled a winnable one)
    `profession` TINYINT UNSIGNED NOT NULL DEFAULT 255, -- PlayerBot::BotProfession (255 = not assigned); persists the bot's job across respawns
    `lastUpdate` DATETIME NOT NULL,
    PRIMARY KEY (`charID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botMemory`;
