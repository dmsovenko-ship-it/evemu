-- +migrate Up
-- Persistent security status per bot (Crucible mechanics):
--   * killing pirate NPCs RAISES it anywhere (cap +5): New = Old + (10-Old)*Inc
--   * PvP kills LOWER it only in high/low sec: New = Old + (10-Old)*Mod%
-- Previously it was derived on the fly from botMemory.kills (-0.5 - 0.9*kills),
-- so NPC kills never raised it. Seed existing bots from that old formula.
ALTER TABLE botMemory ADD COLUMN IF NOT EXISTS securityStatus DOUBLE NOT NULL DEFAULT 0;
UPDATE botMemory SET securityStatus = GREATEST(-10.0, -0.5 - 0.9*kills) WHERE kills > 0;
UPDATE botMemory SET securityStatus = -0.3 WHERE kills = 0 AND deaths > 3;

-- +migrate Down
ALTER TABLE botMemory DROP COLUMN IF EXISTS securityStatus;
