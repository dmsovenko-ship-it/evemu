-- Populate cacheOwners from chrNPCCharacters — required for owner name resolution
-- +migrate Up

REPLACE INTO `cacheOwners` (`ownerID`, `ownerName`, `typeID`, `gender`)
SELECT `characterID`, `characterName`, `typeID`, `gender` FROM `chrNPCCharacters`
WHERE `characterName` IS NOT NULL;

-- +migrate Down
DELETE FROM `cacheOwners` WHERE `ownerID` > 1000000;
