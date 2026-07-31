-- Populate cacheOwners from chrNPCCharacters — required for owner name resolution
-- +migrate Up

REPLACE INTO `cacheOwners` (`ownerID`, `ownerName`, `typeID`)
SELECT `characterID`, `characterName`, `typeID` FROM `chrNPCCharacters`
WHERE `characterName` IS NOT NULL;

-- +migrate Down
DELETE FROM `cacheOwners` WHERE `ownerID` > 1000000;
