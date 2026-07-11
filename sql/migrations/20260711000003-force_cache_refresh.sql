-- +migrate Up
-- Force client to re-fetch config.BulkData.types with remapped groupIDs
-- Update bulkDataChangeID in sysConfig so ObjCacheService detects a change
-- and regenerates all cache objects

-- +migrate Down
