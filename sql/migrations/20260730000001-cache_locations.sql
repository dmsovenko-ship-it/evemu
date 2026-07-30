-- Populate cacheLocations from entity table — required for overview/warp menu to work
-- +migrate Up

REPLACE INTO `cacheLocations` (`locationID`, `locationName`, `x`, `y`, `z`)
SELECT `itemID`, `itemName`, `x`, `y`, `z` FROM `entity`
WHERE `itemName` IS NOT NULL AND `itemName` != '';

-- +migrate Down
DELETE FROM `cacheLocations`;
