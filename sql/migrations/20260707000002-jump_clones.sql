-- Jump clone system: per-clone implants + active clone tracking
-- +migrate Up

-- Per-clone implants table
CREATE TABLE IF NOT EXISTS chrJumpCloneImplants (
    jumpCloneID INT(10) UNSIGNED NOT NULL,
    typeID INT(10) UNSIGNED NOT NULL,
    PRIMARY KEY (jumpCloneID, typeID)
);

-- Track active clone via isActive flag in entity
-- 0 = jump clone, 1 = active clone (where character respawns)
ALTER TABLE entity ADD COLUMN isActive TINYINT(1) NOT NULL DEFAULT 0;

-- Set existing clones as active (they're the only clone most chars have)
UPDATE entity SET isActive = 1 WHERE flag = 400;

-- +migrate Down
DROP TABLE IF EXISTS chrJumpCloneImplants;
ALTER TABLE entity DROP COLUMN isActive;
