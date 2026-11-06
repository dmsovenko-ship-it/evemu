-- +migrate Up
-- Petition extras: message read tracking, ratings, escalation target.
ALTER TABLE `portal_petition_messages`
  ADD COLUMN `isRead` tinyint(1) NOT NULL DEFAULT 0;

ALTER TABLE `portal_petitions`
  ADD COLUMN `responseRating` tinyint(4) NOT NULL DEFAULT 0,
  ADD COLUMN `helpfulnessRating` tinyint(4) NOT NULL DEFAULT 0,
  ADD COLUMN `attitudeRating` tinyint(4) NOT NULL DEFAULT 0,
  ADD COLUMN `ratingComment` varchar(255) NOT NULL DEFAULT '',
  ADD COLUMN `escalatedCategoryID` int(10) unsigned NOT NULL DEFAULT 0;

-- +migrate Down
ALTER TABLE `portal_petition_messages` DROP COLUMN `isRead`;
ALTER TABLE `portal_petitions`
  DROP COLUMN `responseRating`,
  DROP COLUMN `helpfulnessRating`,
  DROP COLUMN `attitudeRating`,
  DROP COLUMN `ratingComment`,
  DROP COLUMN `escalatedCategoryID`;
