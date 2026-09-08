-- +migrate Up
-- Store the admin's ban reason so the client's login error can show it.
ALTER TABLE account ADD COLUMN IF NOT EXISTS banReason varchar(255) NOT NULL DEFAULT '';
-- +migrate Down
ALTER TABLE account DROP COLUMN banReason;
