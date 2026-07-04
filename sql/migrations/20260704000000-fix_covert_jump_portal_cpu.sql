-- Restore this migration as a no-op (was deleted and re-created)
-- The original fix is now part of GenericModule.cpp C++ code and
-- migration 20260705000000 which correctly handles the cloak CPU issue.
-- +migrate Up
-- (no-op, kept for migration chain continuity)
SELECT 1;

-- +migrate Down
SELECT 1;
