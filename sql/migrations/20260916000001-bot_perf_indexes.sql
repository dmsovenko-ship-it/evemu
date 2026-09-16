-- +migrate Up
-- Hot-path indexes for the bot loop (each respawn runs these; without the
-- indexes they scan `entity` (~150k rows) / `botKillmailLegends` (~7k rows)):
--  * skill top-up:      SELECT typeID FROM entity WHERE ownerID=? AND flag=7
--  * hangar stock/cargo SELECT ... FROM entity WHERE ownerID=? AND flag=133
--  * legend restore:    SELECT ... FROM botKillmailLegends WHERE character_name=?
-- IF NOT EXISTS: these may already exist on DBs where the index was applied
-- manually ahead of the migration runner (idempotent re-runs).
CREATE INDEX IF NOT EXISTS idx_entity_owner_flag ON entity (ownerID, flag);
CREATE INDEX IF NOT EXISTS idx_bkl_charname ON botKillmailLegends (character_name);

-- +migrate Down
DROP INDEX IF EXISTS idx_entity_owner_flag ON entity;
DROP INDEX IF EXISTS idx_bkl_charname ON botKillmailLegends;
