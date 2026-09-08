-- +migrate Up
-- Free-form admin comment per account (notes for other GMs/admins).
ALTER TABLE account ADD COLUMN IF NOT EXISTS adminComment varchar(1000) NOT NULL DEFAULT '';
-- +migrate Down
ALTER TABLE account DROP COLUMN adminComment;
