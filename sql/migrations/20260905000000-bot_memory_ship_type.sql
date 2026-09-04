-- Chelobots get a persistent "typical" hull so player lists show a ship even
-- when the bot is offline (its in-space hull is a transient NPC entity, so
-- chrCharacters.shipID is 0 once reaped). BotMgr records the hull type chosen
-- for each bot on spawn here; CharacterList falls back to it for ship display.
-- +migrate Up
ALTER TABLE `botMemory`
    ADD COLUMN `shipTypeID` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `charID`;

-- +migrate Down
ALTER TABLE `botMemory`
    DROP COLUMN `shipTypeID`;
