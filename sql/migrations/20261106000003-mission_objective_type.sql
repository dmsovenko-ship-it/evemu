-- +migrate Up
-- Arc/COSMOS missions have no objective data in the Crucible SDE dump we ship, so
-- the server assigns a deterministic objective per mission (see
-- MissionDataMgr::AssignArcObjective): 1 = travel (visit the destination system),
-- 2 = courier (bring the package to a station there), 3 = hunt (destroy the
-- spawned target cluster).
ALTER TABLE `agtOffers`
  ADD COLUMN `objectiveType` tinyint(4) NOT NULL DEFAULT 0;

-- +migrate Down
ALTER TABLE `agtOffers`
  DROP COLUMN `objectiveType`;
