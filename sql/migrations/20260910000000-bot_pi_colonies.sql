-- +migrate Up
-- Bot-run planetary colonies (Industrialist profession). One colony per bot;
-- the bot extracts P1 and refines up the real PI schematic chain (P2/P3/P4),
-- delivering the output to its hangar/POS.
CREATE TABLE IF NOT EXISTS botColonies (
    charID       INT UNSIGNED NOT NULL,
    planetID     INT UNSIGNED NOT NULL DEFAULT 0,
    schematicID  INT NOT NULL DEFAULT 0,
    lastRun      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (charID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS botColonies;
