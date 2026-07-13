-- Clean up Takmahl Data Registry (typeID 23193) that were spawned as acceleration gates
-- before the typeID fix in 20260711000003-fix_acceleration_gate_typeid.sql
-- +migrate Up
DELETE FROM entity WHERE typeID = 23193 AND customInfo LIKE '%livedungeon%';

-- +migrate Down
-- (cannot restore deleted items)
