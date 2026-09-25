-- +migrate Up
-- Per-moon composition is generated once (first access) and then FIXED forever, so
-- a moon scanned at any time keeps the same products. Rows are individual materials.
CREATE TABLE IF NOT EXISTS moonComposition (
    moonID   INT UNSIGNED NOT NULL,
    typeID   SMALLINT UNSIGNED NOT NULL,
    quantity TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (moonID, typeID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS moonComposition;
