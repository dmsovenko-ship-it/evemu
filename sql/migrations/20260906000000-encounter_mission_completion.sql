-- Encounter mission completion (players + future bot missioners): an encounter
-- offer tracks how many hostile NPC targets it needs cleared vs how many have
-- died. Accept writes the total (dungeon rooms or a spawned destination
-- cluster); NPC::Killed increments the killed counter for the owning offer;
-- Client::IsMissionComplete (Encounter) returns true once killed >= total, so
-- the agent shows the Complete button.
-- +migrate Up
ALTER TABLE `agtOffers`
    ADD COLUMN IF NOT EXISTS `missionNPCs` INT NOT NULL DEFAULT 0 AFTER `dungeonSolarSystemID`,
    ADD COLUMN IF NOT EXISTS `missionNPCsKilled` INT NOT NULL DEFAULT 0 AFTER `missionNPCs`;

-- +migrate Down
ALTER TABLE `agtOffers`
    DROP COLUMN IF EXISTS `missionNPCsKilled`,
    DROP COLUMN IF EXISTS `missionNPCs`;
