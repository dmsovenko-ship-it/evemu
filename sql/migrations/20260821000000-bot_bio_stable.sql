-- One-time bio generation for charbots. The bio is written exactly once (on the
-- pilot's first spawn, right after its profession is rolled) and then frozen, so
-- it stops changing between sessions. bioUpdated=1 means the profession-flavoured
-- bio already exists and must NOT be regenerated.
-- +migrate Up
ALTER TABLE `botMemory` ADD COLUMN `bioUpdated` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `profession`;

-- Existing pilots: their bios were written by older code every respawn. Re-mark
-- them as already updated so they keep whatever bio they currently have — no
-- churn, and the random flip-flopping stops immediately.
UPDATE `botMemory` SET `bioUpdated` = 1;

-- +migrate Down
ALTER TABLE `botMemory` DROP COLUMN `bioUpdated`;
