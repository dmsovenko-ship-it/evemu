-- +migrate Up
-- Persisted friendships / couples between AI pilots. Pairs are stored ordered
-- (charA < charB). BotMgr builds the score as the pilots keep meeting; couples
-- are strictly male/female. Kept in memory as a hot cache and written back
-- occasionally (and on couple formation), so bonds survive restarts.
CREATE TABLE IF NOT EXISTS botBonds (
    charA   INT UNSIGNED NOT NULL,
    charB   INT UNSIGNED NOT NULL,
    score   INT NOT NULL DEFAULT 0,
    couple  TINYINT NOT NULL DEFAULT 0,
    updated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (charA, charB)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS botBonds;
