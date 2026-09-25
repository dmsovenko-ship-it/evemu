-- +migrate Up
-- POS per-structure access settings (view / take / use) are level values 0..3
-- (0=starbase config role, 1=corporation, 2=alliance, 3=fuel/equipment role).
-- They were declared bit(1), so the alliance (2) and role (3) settings were
-- silently truncated to 1/0 on save. Widen to a real integer.
ALTER TABLE posStructureData
  MODIFY canView tinyint(2) NOT NULL DEFAULT 0,
  MODIFY canTake tinyint(2) NOT NULL DEFAULT 0,
  MODIFY canUse  tinyint(2) NOT NULL DEFAULT 0;

-- +migrate Down
ALTER TABLE posStructureData
  MODIFY canView bit(1) NOT NULL DEFAULT b'0',
  MODIFY canTake bit(1) NOT NULL DEFAULT b'0',
  MODIFY canUse  bit(1) NOT NULL DEFAULT b'0';
