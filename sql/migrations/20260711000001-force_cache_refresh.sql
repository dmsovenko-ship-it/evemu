-- Forces cache refresh: Generate_invTypes column count changed (categoryID added/removed)
-- +migrate Up
-- no-op: migration filename timestamp is sufficient to bump bulkDataChangeID
SELECT 1;
-- +migrate Down
SELECT 1;
