-- Fix notificationText.data: must be BLOB not TEXT. The server marshals binary
-- notification payloads (PyMarshal) into this column; a utf8mb4 TEXT column
-- rejects them with "Incorrect string value" (DB error 1366) on every NPC kill,
-- which also triggers a DB reconnect.
-- +migrate Up
ALTER TABLE `notificationText` MODIFY `data` BLOB NOT NULL;

-- +migrate Down
ALTER TABLE `notificationText` MODIFY `data` TEXT NOT NULL;
