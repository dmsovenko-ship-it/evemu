-- +migrate Up
-- cacheOwners had NO primary/unique key, so every ON DUPLICATE KEY UPDATE /
-- INSERT IGNORE site (LSC channel joins, bot creation, char creation) never
-- deduped — 221k rows for ~15.7k owners, all shipped to every client login in
-- config.BulkData.owners (cfg.eveowners). Rebuild the table deduped, add the
-- PK the code always assumed, then seed alliances (never seeded before —
-- clients could not resolve alliance names in mail recipients / owner lists).
CREATE TABLE cacheOwners_dedup AS
    SELECT ownerID, MAX(ownerName) AS ownerName, MAX(typeID) AS typeID, MAX(ownerNameID) AS ownerNameID
    FROM cacheOwners GROUP BY ownerID;
DROP TABLE cacheOwners;
RENAME TABLE cacheOwners_dedup TO cacheOwners;
ALTER TABLE cacheOwners ADD PRIMARY KEY (ownerID);

-- trailing/leading whitespace in names breaks the client's exact-match owner
-- lookup ('Tort Corporation Alliance ' != 'Tort Corporation Alliance').
-- NOTE: PAD SPACE collations ignore trailing spaces in comparisons, so a
-- `WHERE col <> TRIM(col)` guard matches 0 rows — update unconditionally.
UPDATE cacheOwners SET ownerName = TRIM(ownerName);
UPDATE alnAlliance SET allianceName = TRIM(allianceName);

-- seed alliances: typeID 16159 = 'Alliance' (invTypes), same convention as
-- faction 30 / corporation 2 in the 20260907000001 seed
INSERT IGNORE INTO cacheOwners (ownerID, ownerName, typeID)
    SELECT allianceID, allianceName, 16159 FROM alnAlliance;
-- +migrate Down
-- the dedup itself cannot be undone; restore pre-migration shape
ALTER TABLE cacheOwners DROP PRIMARY KEY;
DELETE FROM cacheOwners WHERE ownerID IN (SELECT allianceID FROM alnAlliance) AND typeID = 16159;
