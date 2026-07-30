-- Clean up corrupt entity data left by old decoration systems.
-- These entities have incomplete destiny data and crash MakeSetState.
-- +migrate Up

DELETE FROM `entity` WHERE `itemID` > 14000000
AND `groupID` IN (12, 186, 310, 920, 335, 340, 382, 427, 430, 449, 426);

DELETE FROM `entity` WHERE `itemID` > 14000000
AND `typeID` IN (23,3293,3296,3298,3465,3467,24445,24545,17366,19373,
  10645,10124,10753,10754,10758,10756,
  1225,1226,1227,1228,1229,1230,1231,1232,
  26468,26483,26505,26527,26549,29033,29034,29035,29036);

-- +migrate Down
-- No down migration; data was invalid.
