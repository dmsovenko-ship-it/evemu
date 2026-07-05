-- Fix NULL columns in COSMOS market group entry that crash Market Manager
-- MarketDB::GetMarketGroups() calls row.GetUInt(7-9) on dataID/marketGroupNameID/descriptionID
-- strtoul(NULL, ...) segfaults when these columns are NULL
-- +migrate Up

UPDATE `invMarketGroups` SET
  `graphicID` = 0,
  `iconID` = 0,
  `dataID` = 0,
  `marketGroupNameID` = 0,
  `descriptionID` = 0
WHERE `marketGroupID` = 99000 AND (`dataID` IS NULL OR `marketGroupNameID` IS NULL OR `descriptionID` IS NULL);

-- +migrate Down
-- No down migration needed; data columns can stay non-NULL
