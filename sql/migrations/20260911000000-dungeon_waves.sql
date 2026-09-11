-- +migrate Up
-- Per-NPC wave / trigger data for dungeon rooms.
--   wave      : 1 = spawns together with the site; N>1 = spawns only after wave
--               N-1 has been cleared (or its trigger NPC is killed early).
--   isTrigger : 1 = killing this NPC pulls the next wave early (EVE-Survival:
--               the trigger is usually the battleship / sentinel).
-- Defaults (wave=1, isTrigger=0) keep every existing site exactly as before.
ALTER TABLE `dunRoomObjects`
  ADD COLUMN `wave` TINYINT NOT NULL DEFAULT 1,
  ADD COLUMN `isTrigger` TINYINT NOT NULL DEFAULT 0;

-- W-space Sleeper combat sites (rooms 4001-4099) -> 3-wave pockets.
--   Wave 1 = Patrollers (983/985/987) + Defenders (982/984/986) — the front line
--   Wave 2 = a Sentinel (959/960/961) — the support response
--   Wave 3 = the last Sentinel of the pocket, marked as the trigger (its death
--            pulls wave 3 early, matching the live behaviour)
UPDATE `dunRoomObjects` SET `wave` = 1
  WHERE `roomID` BETWEEN 4001 AND 4099 AND `groupID` IN (982,983,984,985,986,987);

UPDATE `dunRoomObjects` SET `wave` = 2
  WHERE `roomID` BETWEEN 4001 AND 4099 AND `groupID` IN (959,960,961);

UPDATE `dunRoomObjects` o
  JOIN (
      SELECT `roomID`, MAX(`objectID`) AS `mid`
      FROM `dunRoomObjects`
      WHERE `roomID` BETWEEN 4001 AND 4099 AND `groupID` IN (959,960,961)
      GROUP BY `roomID`
  ) m ON o.`objectID` = m.`mid`
  SET o.`wave` = 3, o.`isTrigger` = 1;

-- +migrate Down
ALTER TABLE `dunRoomObjects` DROP COLUMN `isTrigger`, DROP COLUMN `wave`;
