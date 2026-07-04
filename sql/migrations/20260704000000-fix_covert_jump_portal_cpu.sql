-- Fix Covert Jump Portal Generator I CPU requirement
-- It was showing 10000 tf instead of the correct 50 tf for Black Ops ships
-- +migrate Up
UPDATE dgmTypeAttributes
SET valueInt = 50
WHERE typeID = 28652 AND attributeID = 50;

-- Clean up any entity_attributes overrides
DELETE FROM entity_attributes
WHERE attributeID = 50
AND itemID IN (SELECT itemID FROM entity WHERE typeID = 28652);

-- +migrate Down
UPDATE dgmTypeAttributes
SET valueInt = 10000
WHERE typeID = 28652 AND attributeID = 50;
