-- Add missing static data entries and clean up old killmail records
-- +migrate Up
INSERT IGNORE INTO eveStaticOwners (ownerID, ownerName, typeID) VALUES (500022, 'Unknown Faction', 1);
INSERT IGNORE INTO dgmEffects (effectID, effectName, effectCategory, preExpression, postExpression, isOffensive, isAssistance, disallowAutoRepeat, isWarpSafe, guid) VALUES (0, '(none)', 0, 0, 0, 0, 0, 0, 0, 'effects.None');
UPDATE chrKillTable SET moonID = solarSystemID WHERE moonID = 0;
UPDATE chrKillTable SET finalFactionID = 500021 WHERE finalFactionID > 500021;
-- +migrate Down
DELETE FROM eveStaticOwners WHERE ownerID = 500022;
DELETE FROM dgmEffects WHERE effectID = 0;
