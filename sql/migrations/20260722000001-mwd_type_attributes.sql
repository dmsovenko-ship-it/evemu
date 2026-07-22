-- Add dgmTypeAttributes for Mobile Warp Disruptors (group 361)
-- These are missing from the SDE dump.
--
-- Attribute reference:
--   556 = anchoringDelay (ms)
--   676 = unanchoringDelay (ms)
--   677 = onliningDelay (ms)
--   103 = warpScrambleRange (m)
--   105 = warpScrambleStrength
--  1032 = anchoringSecurityLevelMax (0.0 = nullsec only)
--
-- Anchor/unanchor times per user data:
--   Small I = 2 min, Small II = 1 min
--   Medium I = 4 min, Medium II = 2 min
--   Large I = 8 min, Large II = 4 min
-- Unanchor time = same as anchor time.
-- Onlining delay = 60s (standard for Crucible deployables).

-- +migrate Up

-- Mobile Small Warp Disruptor I (12198) — anchor 2min, range 20km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (12198, 556, 120000), (12198, 676, 120000), (12198, 677, 60000),
    (12198, 103, 20000),  (12198, 105, 1),      (12198, 1032, 0);

-- Mobile Medium Warp Disruptor I (12199) — anchor 4min, range 30km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (12199, 556, 240000), (12199, 676, 240000), (12199, 677, 60000),
    (12199, 103, 30000),  (12199, 105, 1),      (12199, 1032, 0);

-- Mobile Large Warp Disruptor I (12200) — anchor 8min, range 40km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (12200, 556, 480000), (12200, 676, 480000), (12200, 677, 60000),
    (12200, 103, 40000),  (12200, 105, 1),      (12200, 1032, 0);

-- Mobile Small Warp Disruptor II (26892) — anchor 1min, range 22km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (26892, 556, 60000), (26892, 676, 60000), (26892, 677, 60000),
    (26892, 103, 22000), (26892, 105, 1),     (26892, 1032, 0);

-- Mobile Medium Warp Disruptor II (26890) — anchor 2min, range 33km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (26890, 556, 120000), (26890, 676, 120000), (26890, 677, 60000),
    (26890, 103, 33000),  (26890, 105, 1),      (26890, 1032, 0);

-- Mobile Large Warp Disruptor II (26888) — anchor 4min, range 44km
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (26888, 556, 240000), (26888, 676, 240000), (26888, 677, 60000),
    (26888, 103, 44000),  (26888, 105, 1),      (26888, 1032, 0);

-- Syndicate Mobile Small Warp Disruptor (28774) — faction, anchor 1min
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (28774, 556, 60000), (28774, 676, 60000), (28774, 677, 60000),
    (28774, 103, 22000), (28774, 105, 1),     (28774, 1032, 0);

-- Syndicate Mobile Medium Warp Disruptor (28772) — faction, anchor 2min
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (28772, 556, 120000), (28772, 676, 120000), (28772, 677, 60000),
    (28772, 103, 33000),  (28772, 105, 1),      (28772, 1032, 0);

-- Syndicate Mobile Large Warp Disruptor (28770) — faction, anchor 4min
INSERT IGNORE INTO dgmTypeAttributes (typeID, attributeID, valueInt) VALUES
    (28770, 556, 240000), (28770, 676, 240000), (28770, 677, 60000),
    (28770, 103, 44000),  (28770, 105, 1),      (28770, 1032, 0);

-- +migrate Down

DELETE FROM dgmTypeAttributes WHERE typeID IN (12198,12199,12200,26888,26890,26892,28770,28772,28774)
    AND attributeID IN (556,676,677,103,105,1032);
