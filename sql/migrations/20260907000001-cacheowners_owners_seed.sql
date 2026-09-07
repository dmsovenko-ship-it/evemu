-- +migrate Up
-- cfg.eveowners (client 'config.BulkData.owners') is generated from cacheOwners
-- (ObjCacheDB::Generate_cacheOwners). Factions and NPC corps were never seeded
-- there, so any repStandings row referencing a faction (500xxx) or NPC corp
-- (1000xxx) made the client's standing list call cfg.eveowners.Get(id) →
-- KeyError → the whole Character Sheet standings window went blank on any char
-- that had standings (agents happened to be inserted by chat/local paths).
-- Seed owners for every faction / corporation / NPC character so the standings
-- (and other owner lists) can resolve names. typeID pins the client's group:
--   30 = 'Faction' (group 19), 2 = 'Corporation' (group 2); NPC characters keep
--   their own character typeID.
INSERT IGNORE INTO cacheOwners (ownerID, ownerName, typeID)
SELECT factionID, factionName, 30 FROM facFactions;
-- Pirate/rogue factions referenced by standings but outside the facFactions
-- 500001..500020 seed set (Rogue Drones use faction 500022 here).
INSERT IGNORE INTO cacheOwners (ownerID, ownerName, typeID) VALUES
    (500021, 'Drones', 30),
    (500022, 'Rogue Swarm', 30);
INSERT IGNORE INTO cacheOwners (ownerID, ownerName, typeID)
SELECT corporationID, corporationName, 2 FROM crpCorporation;
INSERT IGNORE INTO cacheOwners (ownerID, ownerName, typeID)
SELECT characterID, characterName, typeID FROM chrNPCCharacters;
-- +migrate Down
-- Only removes rows this migration could have added (factions we seeded and
-- corporations sourced from crpCorporation). Owner rows added later by the
-- character/chat code are left intact.
DELETE FROM cacheOwners WHERE ownerID BETWEEN 500001 AND 500022 AND typeID = 30;
DELETE FROM cacheOwners WHERE typeID = 2
  AND ownerID IN (SELECT corporationID FROM crpCorporation);
