-- +migrate Up
-- A chelobot that has been killed re-buys its ship fit on the open market with
-- real ISK (see BotMgr resupply). resuppliedDeaths remembers up to which death
-- the bot has already re-fitted, so a respawn only pays after a NEW loss.
ALTER TABLE botMemory ADD COLUMN resuppliedDeaths INT UNSIGNED NOT NULL DEFAULT 0 AFTER deaths;
UPDATE botMemory SET resuppliedDeaths = deaths;
-- +migrate Down
ALTER TABLE botMemory DROP COLUMN resuppliedDeaths;
