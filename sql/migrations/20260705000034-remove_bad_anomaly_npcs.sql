-- Remove NPC room objects from anomaly rooms (migration 33 was rolled back)
-- These typeIDs may not exist in SDE 2021 and cause corrupted entities
-- +migrate Up

DELETE FROM `dunRoomObjects` WHERE `roomID` BETWEEN 2000 AND 2093;

-- +migrate Down
-- No down migration; add NPCs back later with verified typeIDs
