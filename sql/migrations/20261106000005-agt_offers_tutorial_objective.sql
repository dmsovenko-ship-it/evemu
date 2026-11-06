-- +migrate Up
-- Aura tutorial missions reference the tutorial step they track (tutorials.tutorialID).
ALTER TABLE `agtOffers`
  ADD COLUMN `objectiveTutorialID` int(10) unsigned NOT NULL DEFAULT 0;

-- +migrate Down
ALTER TABLE `agtOffers`
  DROP COLUMN `objectiveTutorialID`;
