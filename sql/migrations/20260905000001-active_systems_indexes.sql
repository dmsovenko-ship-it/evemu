-- Speed up ActiveSystems: the portal system list joined chrCharacters and
-- entity without indexes on the join columns (full scans + BNL join buffers
-- made it ~7s, over the portal's 5s API timeout so the page showed empty).
-- +migrate Up
ALTER TABLE `chrCharacters` ADD INDEX `idx_solarsystem` (`solarSystemID`);
ALTER TABLE `entity` ADD INDEX `idx_location_flag` (`locationID`, `flag`);

-- +migrate Down
ALTER TABLE `chrCharacters` DROP INDEX `idx_solarsystem`;
ALTER TABLE `entity` DROP INDEX `idx_location_flag`;
