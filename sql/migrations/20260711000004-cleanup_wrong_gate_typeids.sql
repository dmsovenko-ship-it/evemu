-- Replace Takmahl Data Registry (typeID 23193) with LCS Acceleration Gate (2902)
-- in existing dungeon-spawned entities (pre-fix leftovers)
-- +migrate Up
UPDATE entity SET typeID = 2902 WHERE typeID = 23193 AND customInfo LIKE '%livedungeon%';

-- +migrate Down
UPDATE entity SET typeID = 23193 WHERE typeID = 2902 AND customInfo LIKE '%livedungeon%';
